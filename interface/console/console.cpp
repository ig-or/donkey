

#ifdef WIN32
	#include <SDKDDKVer.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <thread>
#include <iostream>

#include "inproc.h"
#include "eth_client.h"

EthClient cli;
std::chrono::time_point<std::chrono::system_clock> pingTime;

void teeData(char* s, int size) {
	s[size] = 0;
	//printf("\n teeData {%s} \n", s);
	printf("%s", s);
}
void inpData(char* s) {
	//printf(" INPDATA {%s} \n", s);
	cli.do_write(s);
}

void ping() {
	pingTime = std::chrono::system_clock::now();
}


int main(int argc, char *argv[]) {
	//printf("starting .. ");
	//boost::asio::io_context io_context1;
	//boost::asio::ip::tcp::socket s1(io_context1);

	
	using namespace std::chrono_literals;
	pingTime = std::chrono::system_clock::now();
	//printf("console starting .. ");
	//cli.startClient(teeData);
	std::thread tcp([&] { cli.startClient(teeData, ping); } );
	std::unique_lock<std::mutex> lk(mu);
	if (cv.wait_for(lk, 1800ms, [] {return cli.connected; })) {
		printf("tee\n");
	} else {
		printf("cannot connect to server \n");
		cli.StopClient();
		std::this_thread::sleep_for(200ms);
		tcp.join();
		return 0;
	}


	std::thread inp(inputProc, inpData);
	

	//printf("main thread started \n");
	while (!inpExitRequest) {
		std::this_thread::sleep_for(200ms);

		std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
		long long dt = std::chrono::duration_cast<std::chrono::milliseconds>(now - pingTime).count() ;
		if (dt > 3500) {
			printf("ping timeout \r\n");
			inpExitRequest = true;
			printf("111 \r\n");	
			break;
		}
		if (!cli.connected) {
			printf("server disconnected \r\n");
			break;
		}
	}
	printf("222 \r\n");
	inpExitRequest = true;
	//ungetc('q', stdin);
	//printf("exiting .. ");
	//std::cout << "exiting .. ";
	//for (int i = 0; i < 250; i++) { ungetc('q', stdin);   ungetc('\r', stdin);    ungetc('\n', stdin);  }

	printf("stopping tcp .. \r\n");
	cli.StopClient();
	tcp.join();
	printf("tcp thread finished \r\n");

	std::this_thread::sleep_for(200ms);
	inp.join();
	printf("inp thread finished \r\n");

	printf("console stop\r\n");
	return 0;
}


void assert_failed(const char* file, unsigned int line, const char* str) {
	printf("AF: file %s, line %d, (%s)\n", file, line, str);
}

int XQSendInfo(const unsigned char* data, unsigned short int size) {

	return 0;
}