


#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <thread>
#include <iostream>

#include "eth_client.h"
#ifdef G4LIDAR
#include "g4.h"
#endif

#include <stdarg.h>
#include <xmroundbuf.h>

EthClient cli;
std::chrono::time_point<std::chrono::steady_clock> pingTime;
int xmprintf(int q, const char * s, ...);
std::chrono::time_point<std::chrono::steady_clock> eStartTime;

struct PingInfo {
	unsigned char id;
	unsigned int teensyTime;
	std::chrono::time_point<std::chrono::steady_clock> pcTime;
};

XMRoundBuf<PingInfo, 10> pingInfo;

void teeData(char* s, int size) {
	s[size] = 0;
	//xmprintf(0, "\n teeData {%s} \n", s);
	xmprintf(0, "%s", s);
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
	using namespace std::chrono_literals;
	eStartTime = std::chrono::steady_clock::now();

	pingInfo.clear();
	pingTime = std::chrono::steady_clock::now();
	//xmprintf(0, "console starting .. ");

/*  G4 test 
	startG4();
	for (int k = 1; k < 100; k++) {
		std::this_thread::sleep_for(100ms);
	}
	stopG4();

	return 0;
*/

	//cli.startClient(teeData);
	std::thread tcp([&] { cli.startClient(teeData, ping); } );

	std::this_thread::sleep_for(200ms);
	tcp.join();
	return 0;
	

	//xmprintf(0, "main thread started \n");
	while (1) {
		std::this_thread::sleep_for(100ms);

		std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock::now();
		long long dt = std::chrono::duration_cast<std::chrono::milliseconds>(now - pingTime).count(); //  how long ago was the last ping
		if (dt > 1500) {
			xmprintf(0, "ping timeout; dt = %llu milliseconds \r\n", dt);

			//xmprintf(0, "111 \r\n");	
			xmprintf(0, "ping times: \r\n");
			for (int i = 0; i < pingInfo.num; i++) {
				long long dx = std::chrono::duration_cast<std::chrono::milliseconds>(now - pingInfo[i].pcTime).count();
				xmprintf(0, "%d\t(%u, %u, %llu)   \r\n", i, pingInfo[i].id, pingInfo[i].teensyTime, dx);
			}

			break;
		}
		if (!cli.isConnected()) {
			xmprintf(0, "server disconnected \r\n");
			break;
		}
	}
	//xmprintf(0, "222 \r\n");

	//ungetc('q', stdin);
	//xmprintf(0, "exiting .. ");
	//std::cout << "exiting .. ";
	//for (int i = 0; i < 250; i++) { ungetc('q', stdin);   ungetc('\r', stdin);    ungetc('\n', stdin);  }

	xmprintf(0, "stopping tcp .. \r\n");
	cli.StopClient();
	tcp.join();
	xmprintf(0, "tcp thread finished \r\n");

	std::this_thread::sleep_for(200ms);

	xmprintf(0, "eye stop\r\n");
	return 0;
}

void assert_failed(const char* file, unsigned int line, const char* str) {
	xmprintf(0, "AF: file %s, line %d, (%s)\n", file, line, str);
}

int XQSendInfo(const unsigned char* data, unsigned short int size) {

	return 0;
}

static std::mutex xmpMutex;
static const int sbSize = 1024;
static char sbuf[sbSize];
static int currentLogLevel = 7;

int xmprintf(int q, const char * s, ...) {
	std::lock_guard<std::mutex> lk(xmpMutex);
	va_list args;
	va_start(args, s);
	int ok;
	int bs = 0;

	long long t = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - eStartTime).count(); 
	bs = snprintf(sbuf, sbSize - 1, "%.2f \t", (double)(t) * 0.001);

	ok = vsnprintf(sbuf + bs, sbSize - 1 - bs, s, args);
	if ((ok <= 0) || (ok >= (sbSize - bs))) {
		//strcpy(sbuf, " errror 2\n");
		ok = snprintf(sbuf, sbSize - 1, "errror #2, ok = %d \n", ok);
	}

	int eos = strlen(sbuf);
	sbuf[sbSize - 1] = 0;

	if (q <= currentLogLevel) {

	#ifdef WIN32
		printf("%s", sbuf);
	#else
		//#ifdef USE_NCURSES
		//	printw("%s", sbuf);
		//	refresh();
		//#else
			printf("%s", sbuf);
		//#endif
	#endif
	}

	va_end(args);
	return 0;
}
