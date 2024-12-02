

#pragma once

#include "boost/asio.hpp"
#include <mutex>
#include <deque>
#include <atomic>
#include <condition_variable>
//#include <chrono>

//typedef void(*vf)(unsigned char, unsigned int);
//typedef void(*data_1)(char* s, int size);
//typedef void(*data_2)(char* s);

//extern std::mutex mu;
//extern std::condition_variable cv;

class EthConsumer {
	public:
	
	virtual void onConnect(bool connected) {}

	/// incoming eth data; called from eth thread
	virtual void ethData(char* s, int size) {}
	virtual void ethPing(unsigned char, unsigned int) {}

};

/**
 * @brief  TCP ethernet client. 
 * 
 * 
 */
class EthClient {
public:
	EthClient(EthConsumer& consumer_);
	/**
	 * \return true if OK
	*/
	bool startClient(); // , int msTimeout)
	void stopClient();
	/**
	 * \return 
	*/
	int do_write(const char* s);
	int do_write(const unsigned char* buf, int size);
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

	std::atomic<bool> pleaseStop = false;
	
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
	EthConsumer& consumer;

	std::mutex muOutbox;  // mu, 
	//std::condition_variable cv;
	boost::asio::steady_timer deadline_;
	boost::asio::steady_timer heartbeat_timer_;
	std::deque<std::string> outbox_;				///  messages to write

	//  for writing binary info
	static const int smBufSize = 512;
	unsigned char smBuf[smBufSize];
	int smBufIndex = 0;
	
};


