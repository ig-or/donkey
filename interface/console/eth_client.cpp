
#ifdef WIN32
#include <SDKDDKVer.h>
#endif

#include "eth_client.h"
#include <iostream>
//#include <boost/array.hpp>
#include <boost/asio.hpp>
#include "xmutils.h"

int xmprintf(const char * s, ...);

std::mutex mu;
std::condition_variable cv;
static const int ethHeaderSize = 16;

EthClient::EthClient() : socket(io_context) {
	pleaseStop = false;
	connected = false;
	readingTheLogFile = 0;
	theFile = 0;
	pingReply = 0;
	cb1 = 0;
}

void EthClient::startClient(data_1 cb, vf ping) {
	using boost::asio::ip::tcp;
	using boost::asio::ip::address;

	pleaseStop = false;
	connected = false;
	readingTheLogFile = 0;
	theFile = 0;
	lastReportedProgress = 0;
	cb1 = cb;
	ping1 = ping;

	try {
		//tcp::resolver resolver(io_service);
		xmprintf("connecting .... ");
		
		tcp::endpoint ep(address::from_string("192.168.0.177"), 8889);
		
		socket.async_connect(ep,
			[this](boost::system::error_code ec) {
			if (!ec) {
				xmprintf("connected to server\n");
				do_write("console\r\n");
				do_read();
			} else {
				int itmp = 0;
			}
		});
		
		//std::cout << "OK!" << std::endl;  this is not true at this point
		io_context.run();
		
	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
}

bool EthClient::checkThePacket(char* buf, int& len) {
	using namespace std::chrono_literals;
	if (len < ethHeaderSize) { //  too small packet
		xmprintf("checkThePacket: len= %d; packetsCounter = %d\r\n", len, packetsCounter);
		return false;
	}
	if (strncmp(buf, "TBWF", 4) != 0) { // wrong header
		xmprintf("checkThePacket: no header  len = %d;  packetsCounter = %u; buf= {%s}\r\n", len, packetsCounter, buf);
		std::this_thread::sleep_for(2ms);
		int bs;
		while ((bs = socket.available()) > 0) {
			bs = socket.read_some(boost::asio::buffer((char*)(buf), bs), error);
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
			while ((bs = socket.available()) > 0) {
				int haveSpaceInBuffer = bufSize - len;
				int needed = incomingPacketSize + ethHeaderSize - len;
				bs = (bs < haveSpaceInBuffer) ? bs : haveSpaceInBuffer;
				bs = (bs < needed) ? bs : needed;
				bs = socket.read_some(boost::asio::buffer((char*)(buf+len), bs), error);
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
			xmprintf("checkThePacket: incomingPacketSize = %d; (len - ethHeaderSize) = %d; packetsCounter = %d; incomingPacketsCounter=%d\r\n", 
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
		xmprintf("checkThePacket: packetsCounter = %u, incomingPacketsCounter = %u \r\n", packetsCounter, incomingPacketsCounter);
		if (packetsCounter < incomingPacketsCounter) {
			packetsCounter = incomingPacketsCounter;
		}
	}

	return true;
}


int EthClient::do_write(const char* s) {

	
	if (s == 0) {
		return 0;
	}
	int n = strlen(s);
	if (n == 0) {
		return 0;
	}
	//xmprintf("do_write  writing %d bytes\r\n", n);

	if (strncmp(s, "get ", 4) == 0) {
		readingTheLogFile = 1;
		currentFileName = s + 4;
		lastReportedProgress = 0;
		while ((currentFileName.length() > 1) && ((currentFileName.back() == '\n') || (currentFileName.back() == '\r'))) {
			currentFileName.pop_back();
		}
		xmprintf("do_write: reading file %s [%s] \r\n", currentFileName.c_str(), s);
	}

	int ret = socket.write_some(boost::asio::buffer(s, n), error);
	if (ret == 0) {
		readingTheLogFile = 0;
		std::cout << "write error: " << error.message() << std::endl;
	} else {
		//xmprintf("socket.write_some = %d\n", ret);
	}
	return ret;
}

void EthClient::process(boost::system::error_code ec, std::size_t len) {
	if ((ec) || (pleaseStop)) {
		//socket.close();
		xmprintf("EthClient::process error ec=%s; pleaseStop=%s \n", ec.message().c_str(), pleaseStop ? "yes" : "no");
		if ((ec == boost::asio::error::eof) || (ec == boost::asio::error::connection_reset)) {
			connected = false;
			socket.close();
			xmprintf("disconnected ? ");
		}
		return;
	}
	//std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
	if (len < 1) {
		xmprintf("EthClient::process: len = %zd\r\n", len);
		return;
	}
	//if (ping1 != 0) {
	//	ping1();
	//}
	if ((len >= 3) && (strncmp(buf, "tee", 3) == 0)) {   //   connection accepted
		connected = true;
		std::lock_guard<std::mutex> lk(mu);
		cv.notify_all();
		//xmprintf("tee!\n");
		
	} else if ((len >= 4) && (strncmp(buf, "ping", 4) == 0)) { //  ping
		unsigned char id;
		unsigned int time;
		memcpy(&time, buf + 4, 4);
		id = (unsigned char)(buf[8]);
		if (ping1 != 0) {
			ping1(id, time);
		}
	} else if (readingTheLogFile != 0) {
		assert(len < bufSize);
		buf[bufSize-1] = 0;
		readingTheFile(buf, len);
	} else	if (cb1 != 0) {
		cb1(buf, len);
	}
	do_read();
}

void EthClient::readingTheFile(char* buf, int len) {
	using namespace std::chrono_literals;
	//xmprintf("reading len = %d; (%d) \r\n", len, readingTheLogFile);
	const char* sch;
	switch (readingTheLogFile) {
		case 1:
			cb1(buf, len);
			if (((sch = strstr(buf, "ack,")) != NULL) && (sch[4] != 0)) {
				fileSize = 0;
				int k = sscanf(sch + 4, "%d", &fileSize);
				if (k == 1) {
					xmprintf("\t got ack; fileSize = %u \n", fileSize); // << fileSize << std::endl;
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
						int ret = socket.write_some(boost::asio::buffer(buf, ethHeaderSize), error);

						xmprintf("file %s  opened; ret = %d\r\n", currentFileName.c_str(), ret);
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
					xmprintf("skipping the incoming packet #%u; packetsCounter=%u \r\n",
						incomingPacketsCounter, packetsCounter);
					counterErrorCounter += packetsCounter - incomingPacketsCounter;

					//  should we ping it to send another packet now????
					break;
				}
				
				int sizeToSave = incomingPacketSize;
				assert(incomingPacketSize <= len - ethHeaderSize);
				fwrite(buf + ethHeaderSize, 1, sizeToSave, theFile);
				fileBytesCounter += sizeToSave;

				//xmprintf("fwrite incomingPacketsCounter=%d; packetsCounter=%d; len=%d; total=%lld; size=%d\r\n", 
				//	incomingPacketsCounter, packetsCounter, len - ethHeaderSize, fileBytesCounter, incomingPacketSize);
					
				{
					int p = fileBytesCounter * 100 / fileSize;
					if ((p != lastReportedProgress) && ((p % 10) == 0)) {
						lastReportedProgress = p;
						xmprintf("%%%d\r\n", lastReportedProgress);
					}
				}

				if (fileBytesCounter >= fileSize) {
					readingTheLogFile = 0;
					lastReportedProgress = 100;
					fclose(theFile);
					theFile = 0;

					//  send endOfReceive
					std::this_thread::sleep_for(2ms);
					memcpy(buf, "TEEE", 4);   buf[4] = 0;
					int ret = socket.write_some(boost::asio::buffer(buf, ethHeaderSize), error);
					std::this_thread::sleep_for(2ms);
					ret = socket.write_some(boost::asio::buffer(buf, ethHeaderSize), error);


					xmprintf("reading the file finished; fileBytesCounter = %d; fileSize =  %d; incomingPacketsCounter =  %d; packetsCounter =  %d; \
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
					int ret = socket.write_some(boost::asio::buffer(buf, ethHeaderSize), error);
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
					xmprintf("EOT detected; \r\n");

					fclose(theFile);
					theFile = 0;
					xmprintf("reading the file finished; fileBytesCounter = %d; fileSize =  %d; incomingPacketsCounter =  %d; packetsCounter =  %d; badPacketCounter =  %d; counterErrorCounter =  %d\r\n",
						fileBytesCounter, fileSize, incomingPacketsCounter, packetsCounter,
						badPacketCounter, counterErrorCounter);

					readingTheLogFile = 0;
				}

				std::this_thread::sleep_for(2ms);
				size_t bs;
				while ((bs = socket.available()) > 0) {
					bs = (bs < bufSize) ? bs : bufSize;
					bs = socket.read_some(boost::asio::buffer(buf, bs), error);
					xmprintf("got %d bytes (nak) \r\n", bs);
				}

				//  send the NAK:
				++badPacketCounter;
				memset(buf, 0, ethHeaderSize);
				memcpy(buf, "TBWN", 4);
				memcpy(buf + 8, &packetsCounter, 4);
				int ret = socket.write_some(boost::asio::buffer(buf, ethHeaderSize), error);
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
	
	//socket.async_read_some(boost::asio::buffer(buf, bufSize - 2), 
	socket.async_read_some(boost::asio::buffer(buf, 1024),
		[this](boost::system::error_code ec, std::size_t len) { process(ec, len);  });

	if (pleaseStop) {
		return;
	}
	
}


void EthClient::StopClient() {
	pleaseStop = true;
	//socket.close();
	connected = false;
	io_context.post([this]() { 
		socket.close(); 
	});
}	