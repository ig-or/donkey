
#ifdef WIN32
#include <SDKDDKVer.h>
#endif

#define _GLIBCXX_USE_NANOSLEEP

#include "eth_client.h"
#include <iostream>
#include <thread>
//#include <boost/array.hpp>
#include <boost/asio.hpp>
#include "xmutils.h"

using boost::asio::steady_timer;
using namespace std::chrono_literals;
using boost::asio::ip::tcp;
using std::placeholders::_1;
using std::placeholders::_2;


int xmprintf(int q, const char * s, ...);

static const int ethHeaderSize = 16;

EthClient::EthClient(EthConsumer& consumer_): consumer(consumer_), socket_(io_context), deadline_(io_context),  heartbeat_timer_(io_context){
	pleaseStop = false;
	connectedToTeensy = false;
	eState = esInit;

	// log file --------------
	readingTheLogFile = 0;
	theFile = 0;
}

bool EthClient::startClient(){ // , int msTimeout
	using boost::asio::ip::tcp;
	using boost::asio::ip::address;

	pleaseStop = false;
	eState = esConnecting;
	//

	// --- log file --
	readingTheLogFile = 0;
	theFile = 0;
	lastReportedProgress = 0;
	outbox_.clear();

	xmprintf(5, "EthClient::startClient() starting \n");

	try {
		//tcp::resolver resolver(io_service);
		
		xmprintf(9, "\tconnecting to teensy.... \n");
		tcp::endpoint ep(address::from_string("192.168.0.177"), 8889);

		start_connect(ep);

		// Start the deadline actor. You will note that we're not setting any
		// particular deadline here. Instead, the connect and input actors will
		// update the deadline prior to each asynchronous operation.
		deadline_.async_wait(std::bind(&EthClient::check_deadline, this));
		io_context.run();
		
	} catch (std::exception& e) {
		xmprintf(1, "ERROR: cannot connect to tennsy; exeption: %s  \r\n", e.what());
		return false;
	}
	xmprintf(6, "EthClient::startClient() finished (io_context.run() exited)\n");
	return true;
}

void EthClient::start_connect(const boost::asio::ip::tcp::endpoint& ep) {
      // Set a deadline for the connect operation.
	  eState = esConnecting;
	  connectedToTeensy = false;
      deadline_.expires_after(std::chrono::milliseconds(5000));

      // Start the asynchronous connect operation.
	  xmprintf(9, "\tEthClient::start_connect(); starting socket_.async_connect() \n");
      socket_.async_connect(ep, std::bind(&EthClient::handle_connect, this, _1, ep));
}

void EthClient::handle_connect(const boost::system::error_code& error,  const boost::asio::ip::tcp::endpoint& ep) {
	xmprintf(9, "---------EthClient::handle_connect() started; error = %s -------------\n", error.message().c_str());

	if (pleaseStop)
      return;

    // The async_connect() function automatically opens the socket at the start
    // of the asynchronous operation. If the socket is closed at this time then
    // the timeout handler must have run first.
    if (!socket_.is_open())   {
      xmprintf(8, "\tEthClient::handle_connect() socket was closed.  Connect timed out; will try again\n");

      // Try again!
      start_connect(ep);
    }  else if (error) { // Check if the connect operation failed before the deadline expired.
		xmprintf(8, "\tEthClient::handle_connect()   Connect error: %s;  will close the socket and try again in 5 seconds\n", error.message().c_str());

		// We need to close the socket used in the previous connection attempt
		// before starting a new one.
		socket_.close();

		// Try again
		//std::this_thread::sleep_for(2000ms);
		//start_connect(ep);
		heartbeat_timer_.expires_after(std::chrono::milliseconds(5000));
		heartbeat_timer_.async_wait(std::bind(&EthClient::start_connect, this, ep));
		deadline_.expires_at(steady_timer::time_point::max());  // start_connect will reset it soon
    } else {    // Otherwise we have successfully established a connection.
		xmprintf(7, "\tEthClient::handle_connect()   Connected to %s \n", ep.address().to_string().c_str());
		eState = esConnected;
		outbox_.clear();
		do_read();// Start the input actor.
		start_write();// Start the heartbeat actor.
		do_write("console ");
    }
}

void EthClient::check_deadline() {
	xmprintf(9, "---EthClient::check_deadline();--state = %d\n", (int)(eState));
	if (pleaseStop) {
		return;
	}

	// Check whether the deadline has passed. We compare the deadline against
	// the current time since a new asynchronous operation may have moved the
	// deadline before this actor had a chance to run.
	if (deadline_.expiry() <= steady_timer::clock_type::now()) 	{
		// The deadline has passed. The socket is closed so that any outstanding
		// asynchronous operations are cancelled.
		xmprintf(9, "\tcheck_deadline(); The deadline has passed. closing the socket\n");
		if (eState == esConnected) {
			xmprintf(4, "EthClient::check_deadline() disconnected fron teensy (ping timeout) \n");
		}
		socket_.close();

		// There is no longer an active deadline. The expiry is set to the
		// maximum time point so that the actor takes no action until a new
		// deadline is set.
		deadline_.expires_at(steady_timer::time_point::max());

		//deadline_.expires_after(std::chrono::milliseconds(2500));
	}

	// Put the actor back to sleep.
	xmprintf(9, "\tcheck_deadline(); Put the actor back to sleep.\n");
	deadline_.async_wait(std::bind(&EthClient::check_deadline, this));
}


bool EthClient::checkThePacket(char* buf, int& len) {
	using namespace std::chrono_literals;
	if (len < ethHeaderSize) { //  too small packet
		xmprintf(0, "checkThePacket: len= %d; packetsCounter = %d\r\n", len, packetsCounter);
		return false;
	}
	if (strncmp(buf, "TBWF", 4) != 0) { // wrong header
		xmprintf(0, "checkThePacket: no header  len = %d;  packetsCounter = %u; buf= {%s}\r\n", len, packetsCounter, buf);
		std::this_thread::sleep_for(2ms);
		int bs;
		while ((bs = socket_.available()) > 0) {
			bs = socket_.read_some(boost::asio::buffer((char*)(buf), bs), error);
		}
		return false;
	}

	memcpy(&incomingPacketsCounter, buf + 8, 4);
	memcpy(&incomingPacketSize, buf + 4, 4);
	if (incomingPacketSize != (len - ethHeaderSize)) {
		
		static const int tc = 5;
		int tCounter = 0;
		size_t bs = 0;

		do {
			std::this_thread::sleep_for(2ms);
			while ((bs = socket_.available()) > 0) {
				int haveSpaceInBuffer = bufSize - len;
				int needed = incomingPacketSize + ethHeaderSize - len;
				bs = (bs < haveSpaceInBuffer) ? bs : haveSpaceInBuffer;
				bs = (bs < needed) ? bs : needed;
				bs = socket_.read_some(boost::asio::buffer((char*)(buf+len), bs), error);
				//bs = socket.read_some((buf+len, bs), error);
				len += bs;

				if (len >= (incomingPacketSize +ethHeaderSize)) {
					break;
				}
			}
			if (bs < 1) {
				break;
			}
			tCounter++;
		} while (tCounter < tc);

		if (len >= (incomingPacketSize +ethHeaderSize)) {
		} else {
			xmprintf(0, "checkThePacket: incomingPacketSize = %d; (len - ethHeaderSize) = %d; packetsCounter = %d; incomingPacketsCounter=%d\r\n", 
				incomingPacketSize, (len - ethHeaderSize), packetsCounter, incomingPacketsCounter);
			
			return false;
		}
	}
	
	unsigned short int realCRC = CRC2::crc16(0, buf + ethHeaderSize, incomingPacketSize);
	unsigned short int incomingCRC;
	memcpy(&incomingCRC, buf + 12, 2);
	if (incomingCRC != realCRC) {
		std::cout << "checkThePacket: crc error" << "; packetsCounter = " << packetsCounter << std::endl;
		return false;
	}
	if ((buf[14] != 0) || (buf[15] != 0)) {
		std::cout << "checkThePacket: EOH error" << "; packetsCounter = " << packetsCounter << std::endl;
		return false; 
	}
	if (packetsCounter != incomingPacketsCounter) {
		xmprintf(0, "checkThePacket: packetsCounter = %u, incomingPacketsCounter = %u \r\n", packetsCounter, incomingPacketsCounter);
		if (packetsCounter < incomingPacketsCounter) {
			packetsCounter = incomingPacketsCounter;
		}
	}

	return true;
}

/*	static const int smBufSize = 512;
	unsigned char smBuf[smBufSize];
	int smBufIndex = 0;
	*/
int EthClient::do_write(const unsigned char* buf, int size) {
	xmprintf(9, "EthClient::do_write()  size = %d bytes; connectedToTeensy=%s\n", size, connectedToTeensy?"yes":"no");
	if ((buf == 0) || (size < 1)) {
		return 0;
	}
	if (!connectedToTeensy) {
		return 0;
	}

	std::lock_guard<std::mutex> lk(muOutbox);
	int u = smBufSize - smBufIndex - 1; // the space that we have
	if (u < 10) {
		xmprintf(1, "EthClient::do_write overflow 1; u = %d \n", u);
		return 0;
	}
	if (u < size) { //  overflow
		xmprintf(1, "EthClient::do_write overflow 2; u = %d \n", u);
		return 0;
	} else { // OK
		u = size;
	}
	memcpy(smBuf + smBufIndex, buf, u);
	smBufIndex += u;
	xmprintf(9, "\tEthClient::do_write()  smBufIndex = %d;   canceling  heartbeat_timer_\n", smBufIndex);
	boost::asio::post(io_context,   [this]()   {   heartbeat_timer_.cancel();   });
	return 0;
}

int EthClient::do_write(const char* s) {
	if (s == 0) {
		return 0;
	}
	int n = strlen(s);
	if (n == 0) {
		return 0;
	}
	//xmprintf(0, "do_write  writing %d bytes\r\n", n);

	if (strncmp(s, "get ", 4) == 0) { //  log file handling
		readingTheLogFile = 1;
		currentFileName = s + 4; //  exactly one space after 'get'
		lastReportedProgress = 0;
		// adjust the file name
		while ((currentFileName.length() > 1) && ((currentFileName.back() == '\n') || (currentFileName.back() == '\r'))) {
			currentFileName.pop_back();
		}
		xmprintf(1, "do_write: reading file %s [%s] \r\n", currentFileName.c_str(), s);
	}

	std::lock_guard<std::mutex> lk(muOutbox);
	outbox_.push_back(s);
	boost::asio::post(io_context,   [this]()   {   heartbeat_timer_.cancel();   });
	

	//int ret = socket_.write_some(boost::asio::buffer(s, n), error); 

	return 0;
}

void EthClient::makeReconnect() {
		socket_.close();
		xmprintf(4, "\t(re)connecting to teensy.... \n");
		tcp::endpoint ep(boost::asio::ip::address::from_string("192.168.0.177"), 8889);
		deadline_.cancel();
		heartbeat_timer_.cancel();
		start_connect(ep);
}

void EthClient::process(boost::system::error_code ec, std::size_t len) {
	xmprintf(9, "EthClient::process starting (eState = %d) \n", eState);
	if (pleaseStop) {
		return;
	}
	if (eState != esConnected) {
		xmprintf(5, "\tEthClient::process : not connected; \n");
		return;
	}
	if (ec) {
		//socket.close();
		xmprintf(4, "\tEthClient::process error ec=%s; pleaseStop=%s \n", ec.message().c_str(), pleaseStop ? "yes" : "no");
		using namespace boost::asio::error;
		switch(ec.value()) {
			case boost::asio::error::eof:
			case connection_reset:
			case connection_aborted:
			case host_unreachable:
			case interrupted:
			case network_down:
			case network_unreachable:
			case not_connected:
				xmprintf(4, " disconnected ? \n");
		};
		makeReconnect(); 
		return;
	}
	//std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
	if (len < 1) {
		xmprintf(1, "\tEthClient::process: len = %zd\r\n", len);
		return;
	}
	//if (ping1 != 0) {
	//	ping1();
	//}
	if ((len >= 3) && (strncmp(buf, "tee", 3) == 0)) {   //   connection accepted
		connectedToTeensy = true;
		std::lock_guard<std::mutex> lk(mu);
		cv.notify_all();
		xmprintf(1, "tee! connection accepted by teensy\n");
		
	} else if ((len >= 9) && (strncmp(buf, "ping", 4) == 0)) { //  ping
		unsigned char id;
		unsigned int time;
		memcpy(&time, buf + 4, 4);
		id = (unsigned char)(buf[8]);
		//if (ping1 != 0) {
		//	ping1(id, time);
		//}
		consumer.ethPing(id, time);
		xmprintf(9, "\t got ping[%u] from teensy\n", id);
	} else if (readingTheLogFile != 0) {
		assert(len < bufSize);
		buf[bufSize-1] = 0;
		readingTheFile(buf, len);
	} else{
		
		//if (cb1 != 0) {
		//	cb1(buf, len);
		//}
		consumer.ethData(buf, len);
		buf[bufSize-1] = 0;
		xmprintf(10, "\t got %d bytes (%s)\n", len, buf);
	}
	do_read();
}

void EthClient::readingTheFile(char* buf, int len) {
	using namespace std::chrono_literals;
	//xmprintf(0, "reading len = %d; (%d) \r\n", len, readingTheLogFile);
	const char* sch;
	switch (readingTheLogFile) {
		case 1:
			//cb1(buf, len);
			consumer.ethData(buf, len);
			if (((sch = strstr(buf, "ack,")) != NULL) && (sch[4] != 0)) {
				fileSize = 0;
				int k = sscanf(sch + 4, "%d", &fileSize);
				if (k == 1) {
					xmprintf(0, "\t got ack; fileSize = %u \n", fileSize); // << fileSize << std::endl;
					theFile = fopen(currentFileName.c_str(), "wb"); //  TODO: do this BEFORE sending the request to the TEENSY
					if (theFile == 0) {
						readingTheLogFile = 0;
						std::cout << "file write error: file " << currentFileName << " is probably locked" << std::endl  << std::endl;
						//gfState = 0;

					} else { //  all is good
						readingTheLogFile = 2;
						fileBytesCounter = 0;
						packetsCounter = 1;     ///   starting from #1
						badPacketCounter = 0;
						counterErrorCounter = 0;

						memset(buf, 0, ethHeaderSize);
						memcpy(buf, "TBWF", 4);// memcpy(buf + 8, &packetsCounter, 4);
						int ret = socket_.write_some(boost::asio::buffer(buf, ethHeaderSize), error);

						xmprintf(0, "file %s  opened; ret = %d\r\n", currentFileName.c_str(), ret);
						//ret = socket.write_some(boost::asio::buffer(buf, ethHeaderSize), error);
						//ret = socket.write_some(boost::asio::buffer(buf, ethHeaderSize), error);
					}
				} else { //  cannot parce answer
					std::cout << "reply parsing error: got " << buf << std::endl;
					readingTheLogFile = 0;
				}

				break;
			}
			if (((sch = strstr(buf, "nak,")) != NULL)) {
				readingTheLogFile = 0;
				std::cout << "cannot get this file (got NAK) " << std::endl;
				break;
			}

			break;

		case 2:
			if (checkThePacket(buf, len)) {

				if (incomingPacketsCounter < packetsCounter) { //  looks like we got this already
					xmprintf(0, "skipping the incoming packet #%u; packetsCounter=%u \r\n",
						incomingPacketsCounter, packetsCounter);
					counterErrorCounter += packetsCounter - incomingPacketsCounter;

					//  should we ping it to send another packet now????
					break;
				}
				
				int sizeToSave = incomingPacketSize;
				assert(incomingPacketSize <= len - ethHeaderSize);
				fwrite(buf + ethHeaderSize, 1, sizeToSave, theFile);
				fileBytesCounter += sizeToSave;

				//xmprintf(0, "fwrite incomingPacketsCounter=%d; packetsCounter=%d; len=%d; total=%lld; size=%d\r\n", 
				//	incomingPacketsCounter, packetsCounter, len - ethHeaderSize, fileBytesCounter, incomingPacketSize);
					
				{
					int p = fileBytesCounter * 100 / fileSize;
					if ((p != lastReportedProgress) && ((p % 10) == 0)) {
						lastReportedProgress = p;
						xmprintf(0, "%%%d\r\n", lastReportedProgress);
					}
				}

				if (fileBytesCounter >= fileSize) {
					readingTheLogFile = 0;
					lastReportedProgress = 100;
					fclose(theFile);
					theFile = 0;

					//  send endOfReceive
					std::this_thread::sleep_for(2ms);
					std::this_thread::sleep_for(std::chrono::milliseconds(2));
					memcpy(buf, "TEEE", 4);   buf[4] = 0;
					int ret = socket_.write_some(boost::asio::buffer(buf, ethHeaderSize), error);
					std::this_thread::sleep_for(2ms);
					ret = socket_.write_some(boost::asio::buffer(buf, ethHeaderSize), error);


					xmprintf(0, "reading the file finished; fileBytesCounter = %d; fileSize =  %d; incomingPacketsCounter =  %d; packetsCounter =  %d; \
							badPacketCounter =  %d; counterErrorCounter =  %d\r\n",
						fileBytesCounter, fileSize, incomingPacketsCounter, packetsCounter,
						badPacketCounter, counterErrorCounter);

					//using namespace boost::filesystem;
					//path e = path(currentFileName).extension();
					//if (e.string().compare(".log") == 0) { //  this was a log file
					//	parceLogFile(currentFileName.c_str());
					//}
				}	else {
					//  send the ACK:
					memset(buf, 0, ethHeaderSize);
					memcpy(buf, "TBWF", 4);    memcpy(buf + 8, &packetsCounter, 4);
					++packetsCounter;
					int ret = socket_.write_some(boost::asio::buffer(buf, ethHeaderSize), error);
					if (ret == 0) {
						readingTheLogFile = 0;
						std::cout << "write error while sending ACK to the teensy: " << error.message() << std::endl;
					}
					else { //  OK!
					}

				}
			} else {
				// ?
				if (memcmp(buf, "TEEE", 4) == 0) {
					xmprintf(0, "EOT detected; \r\n");

					fclose(theFile);
					theFile = 0;
					xmprintf(0, "reading the file finished; fileBytesCounter = %d; fileSize =  %d; incomingPacketsCounter =  %d; packetsCounter =  %d; badPacketCounter =  %d; counterErrorCounter =  %d\r\n",
						fileBytesCounter, fileSize, incomingPacketsCounter, packetsCounter,
						badPacketCounter, counterErrorCounter);

					readingTheLogFile = 0;
				}

				std::this_thread::sleep_for(2ms);
				size_t bs;
				while ((bs = socket_.available()) > 0) {
					bs = (bs < bufSize) ? bs : bufSize;
					bs = socket_.read_some(boost::asio::buffer(buf, bs), error);
					xmprintf(0, "got %d bytes (nak) \r\n", bs);
				}

				//  send the NAK:
				++badPacketCounter;
				memset(buf, 0, ethHeaderSize);
				memcpy(buf, "TBWN", 4);
				memcpy(buf + 8, &packetsCounter, 4);
				int ret = socket_.write_some(boost::asio::buffer(buf, ethHeaderSize), error);
				if (ret == 0) {
					readingTheLogFile = 0;
					std::cout << "write error while sending NAK to the teensy: " << error.message() << std::endl;
				} else { //  OK!
				}
			}
			break;
		default: break;
	};

}

void EthClient::do_read() {
	if (pleaseStop) {
		return;
	}
	if (eState != esConnected) {
		xmprintf(5, "EthClient::do_read : not connected; \n");
		return;
	}

	deadline_.expires_after(std::chrono::milliseconds(850));
	//socket.async_read_some(boost::asio::buffer(buf, bufSize - 2), 
	xmprintf(9, "EthClient::do_read(); starting socket_.async_read_some   \n");
	socket_.async_read_some(boost::asio::buffer(buf, 1024),
		[this](boost::system::error_code ec, std::size_t len) { process(ec, len);  });
}

void EthClient::start_write()  {
	if (pleaseStop) {
		xmprintf(5, "\tEthClient::start_write : pleaseStop!\n");
		return;
	}
	if (eState != esConnected) {
		xmprintf(5, "\tEthClient::start_write : not connected; \n");
		return;
	}

	xmprintf(9, "EthClient::start_write()  Start an asynchronous operation to send a message. \n");
	std::string s;   //   a string to write
	std::lock_guard<std::mutex> lk(muOutbox);
	if (outbox_.empty() && (smBufIndex == 0)) {
		s = "ping";
		xmprintf(9, "EthClient::start_write()  sending a ping \n");
		boost::asio::async_write(socket_, boost::asio::buffer(s), std::bind(&EthClient::handle_write, this, _1));
	} else {
		if (smBufIndex != 0) {
			xmprintf(9, "EthClient::start_write()  sending binary info, smBufIndex = %d \n", smBufIndex); 
			boost::asio::async_write(socket_, boost::asio::buffer(smBuf, smBufIndex), std::bind(&EthClient::handle_write, this, _1));
			smBufIndex = 0;
		} else if (!outbox_.empty()) {
			s = outbox_.front();
			outbox_.pop_front();
			xmprintf(9, "EthClient::start_write()  sending a string from outbox_\n");
			boost::asio::async_write(socket_, boost::asio::buffer(s), std::bind(&EthClient::handle_write, this, _1));
		}
	}
}

void EthClient::handle_write(const boost::system::error_code& error)   {
	xmprintf(9, "EthClient::handle_write(error = %s) \n", error.message().c_str());
	if (pleaseStop) {
		xmprintf(9, "\tEthClient::handle_write pleaseStop! \n");
		return;
	}
	if (eState != esConnected) {
		xmprintf(5, "\tEthClient::handle_write : not connected; \n");
		return;
	}

	if (!error)     {
		
		muOutbox.lock();
		bool oEmpty = outbox_.empty() && (smBufIndex == 0);
		muOutbox.unlock();
		if (oEmpty) {			// nothing to send right now. 
			xmprintf(9, "\tEthClient::handle_write: no error, waiting..  \n");
			heartbeat_timer_.expires_after(std::chrono::milliseconds(380));
			heartbeat_timer_.async_wait(std::bind(&EthClient::start_write, this));
		}  else {
			xmprintf(9, "\tEthClient::handle_write: no error, calling  start_write() \n");
			start_write();	// there is something in the queue
		}
	}  else {
		if (error.value() ==  boost::asio::error::operation_aborted) { // we just add something to write maybe
			xmprintf(9, "\tEthClient::handle_write: error = operation_aborted, so queue is not empty\n");
			start_write();
		} else {  //   serious error like disconnect
			xmprintf(4, "EthClient::handle_write(Error on heartbeat: error = %s) \n", error.message().c_str());
			socket_.close();
			xmprintf(4, "\tdisconnected ? makeReconnect!\n");

			makeReconnect();
		}
	}
}

void EthClient::StopClient() {
	pleaseStop = true;
	//socket.close();
	connectedToTeensy = false;

    boost::system::error_code ignored_error;
    deadline_.cancel();
    heartbeat_timer_.cancel();

	io_context.post([this]() { 
		try {
		socket_.shutdown(boost::asio::socket_base::shutdown_both);
		socket_.close(); 
		} catch (const std::exception& ex) {
			//  actually socket_ is bad already
		}
	});
	socket_.close(ignored_error);
}	