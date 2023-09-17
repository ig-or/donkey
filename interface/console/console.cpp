

#ifdef WIN32
	#include <SDKDDKVer.h>
#else
	//#include <ncurses.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <thread>
#include <iostream>

#include "inproc.h"
#include "eth_client.h"
#include <stdarg.h>
#include <xmroundbuf.h>

EthClient cli;
std::chrono::time_point<std::chrono::steady_clock> pingTime;
int xmprintf(const char * s, ...);

struct PingInfo {
	unsigned char id;
	unsigned int teensyTime;
	std::chrono::time_point<std::chrono::steady_clock> pcTime;
};

XMRoundBuf<PingInfo, 10> pingInfo;

void teeData(char* s, int size) {
	s[size] = 0;
	//xmprintf("\n teeData {%s} \n", s);
	xmprintf("%s", s);
}
void inpData(char* s) {
	//xmprintf(" INPDATA {%s} \n", s);
	cli.do_write(s);
}

void ping(unsigned char id, unsigned int time) {
	pingTime = std::chrono::steady_clock::now();
	PingInfo pInfo;
	pInfo.id = id;
	pInfo.teensyTime = time;
	pInfo.pcTime = pingTime;
	pingInfo.put(pInfo);
}


int main(int argc, char *argv[]) {
	//xmprintf("starting .. ");
	//boost::asio::io_context io_context1;
	//boost::asio::ip::tcp::socket s1(io_context1);

	#ifndef WIN32

	#endif

	
	using namespace std::chrono_literals;
	std::thread inp(inputProc, inpData);
	pingInfo.clear();
	pingTime = std::chrono::steady_clock::now();
	//xmprintf("console starting .. ");
	//cli.startClient(teeData);
	std::thread tcp([&] { cli.startClient(teeData, ping); } );
	std::unique_lock<std::mutex> lk(mu);
	if (cv.wait_for(lk, 1800ms, [] {return cli.connected; })) {
		xmprintf("tee\n");
	} else {
		xmprintf("cannot connect to server \n");
		cli.StopClient();
		std::this_thread::sleep_for(200ms);
		tcp.join();
		return 0;
	}
	//xmprintf("main thread started \n");
	while (!inpExitRequest) {
		std::this_thread::sleep_for(200ms);

		std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock::now();
		long long dt = std::chrono::duration_cast<std::chrono::milliseconds>(now - pingTime).count() ;
		if (dt > 1500) {
			xmprintf("ping timeout; dt = %llu milliseconds \r\n", dt);
			inpExitRequest = true;
			//xmprintf("111 \r\n");	
			xmprintf("ping times: \r\n");
			for (int i = 0; i < pingInfo.num; i++) {
				long long dx = std::chrono::duration_cast<std::chrono::milliseconds>(now - pingInfo[i].pcTime).count();
				xmprintf("%d\t(%u, %u, %llu)   \r\n", i, pingInfo[i].id, pingInfo[i].teensyTime, dx);
			}

			break;
		}
		if (!cli.connected) {
			xmprintf("server disconnected \r\n");
			break;
		}
	}
	//xmprintf("222 \r\n");
	inpExitRequest = true;
	//ungetc('q', stdin);
	//xmprintf("exiting .. ");
	//std::cout << "exiting .. ";
	//for (int i = 0; i < 250; i++) { ungetc('q', stdin);   ungetc('\r', stdin);    ungetc('\n', stdin);  }

	xmprintf("stopping tcp .. \r\n");
	cli.StopClient();
	tcp.join();
	xmprintf("tcp thread finished \r\n");

	std::this_thread::sleep_for(200ms);
	inp.join();
	xmprintf("inp thread finished \r\n");

	xmprintf("console stop\r\n");
	return 0;
}

void assert_failed(const char* file, unsigned int line, const char* str) {
	xmprintf("AF: file %s, line %d, (%s)\n", file, line, str);
}

int XQSendInfo(const unsigned char* data, unsigned short int size) {

	return 0;
}