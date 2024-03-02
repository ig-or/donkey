

#pragma once

#include "boost/asio.hpp"
#include <mutex>
#include <condition_variable>
//#include <chrono>

typedef void(*vf)(unsigned char, unsigned int);
typedef void(*data_1)(char* s, int size);
typedef void(*data_2)(char* s);

//extern std::mutex mu;
//extern std::condition_variable cv;

/**
 * @brief  TCP ethernet client. 
 * 
 * 
 */
class EthClient {
public:
	EthClient();
	/**
	 * \return true if OK
	*/
	bool startClient(data_1 cb, vf ping); // , int msTimeout)
	void StopClient();
	/**
	 * \return 
	*/
	int do_write(const char* s);
	bool isConnected() {
		return connectedToTeensy;
	}

private:
enum EState {
	esInit,
	esConnecting, 
	esConnected
};
	EState eState = esInit;
	bool connectedToTeensy = false;
	vf pingReply = 0;				///  this will be called on a ping arrival

	//  -------------for log file downloading----------------------
	int readingTheLogFile = 0;
	int lastReportedProgress = 0;
	int fileSize, fileBytesCounter, packetsCounter, badPacketCounter, counterErrorCounter;
	int incomingPacketsCounter = 0;
	int incomingPacketSize = 0;
	std::string currentFileName;
	FILE*	theFile = 0;
	bool checkThePacket(char* buf, int& len);
	void readingTheFile(char* buf, int len);
	// -----------------------------------------------------------------

	volatile bool pleaseStop = false;
	
	boost::asio::io_context io_context;	
	boost::asio::ip::tcp::socket socket_;
	static const int bufSize = 2050;
	char buf[bufSize];
	boost::system::error_code error;
	
	void do_read();
	void start_connect(const boost::asio::ip::tcp::endpoint& ep);
	void handle_connect(const boost::system::error_code& error,  const boost::asio::ip::tcp::endpoint& ep);
	void check_deadline();
	void handle_write(const boost::system::error_code& error);
	void start_write();
	void makeReconnect();
	void process(boost::system::error_code ec, std::size_t len);
	data_1 cb1 = 0;
	vf ping1 = 0;

	std::mutex mu;
	std::condition_variable cv;
	boost::asio::steady_timer deadline_;
	boost::asio::steady_timer heartbeat_timer_;
	
};


