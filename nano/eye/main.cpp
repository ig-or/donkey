


#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <thread>
#include <iostream>

#include <signal.h>
#include <unistd.h>
#include <stdarg.h>

#include "eth_client.h"
#include "lidar.h"
#include <xmroundbuf.h>
#include "xmessagesend.h"
#include "xmessage.h"
#ifdef G4LIDAR
#include "g4.h"
#endif
#include "eye.h"

static EthClient* cli = 0;
static SLidar* lidar = 0;
static Eye e;
static int currentLogLevel = 7; // 7

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


void exitHandler(int s){
	xmprintf(0, "exitHandler() Caught signal %d\n",s);

	if (lidar) {
		bool result = lidar->stopLidar();
		xmprintf(6, "exitHandler() lidar stopped: %s \n", result ? "yes" : "no");
		delete lidar; lidar = 0;
	}
	if (cli) {
		cli->StopClient();
		delete cli; cli = 0;
	}
	e.stopEye();
	xmprintf(6, "exitHandler() filished\n");
	exit(0); 
}

int main(int argc, char *argv[]) {
	using namespace std::chrono_literals;
	bool result;
	eStartTime = std::chrono::steady_clock::now();
	pingInfo.clear();
	pingTime = std::chrono::steady_clock::now();
	xmprintf(0, "starting .. currentLogLevel = %d\n", currentLogLevel);

	// setup Ctrl^C
	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = exitHandler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(SIGINT, &sigIntHandler, NULL);
 
	cli = new EthClient(e);
	std::thread tcp([&] { cli->startClient(); } );
	
	#ifdef G4LIDAR
		lidar = new G4Lidar(e);
	#else
		lidar = new SLidar(e);  //this will do nothing
	#endif
	e.setEthClient(cli);
	result = lidar->startLidar();

	std::this_thread::sleep_for(200ms);
	e.startEye();
	

	//xmprintf(0, "stopping tcp .. \r\n");
	//cli->StopClient();
	//tcp.join();
	//xmprintf(0, "tcp thread finished \r\n");
	
	while(1) {
		std::this_thread::sleep_for(200ms);
	}

	xmprintf(0, "eye stop\r\n");
	return 0;
}

void assert_failed(const char* file, unsigned int line, const char* str) {
	xmprintf(0, "AF: file %s, line %d, (%s)\n", file, line, str);
}

// -----------------------------------------------------------------------------------------------------------------------

static std::mutex msgSendMutex;
const int smBufSize = maxxMessageSize*2;
unsigned char smBuf[smBufSize];
unsigned int smBufIndex = 0;
void sendMsg(const unsigned char* data, unsigned char type, unsigned short int size) {
	if (cli == 0) {
		return;
	}
	std::lock_guard<std::mutex> lk(msgSendMutex);   //  this might be called from different threads
	smBufIndex = 0;
	sendMessage(data, type, size);
	//if (cli != 0) {
		cli->do_write(smBuf, smBufIndex);
	//}
}

/**
 * this is called from inside 'sendMessage' several times. 
 * This function puts data into smBuf
*/
int XQSendInfo(const unsigned char* data, unsigned short int size) {
	int u  = smBufSize - smBufIndex - 1;
	if (u < size) { // space available smaller than we need
		xmprintf(1, "XQSendInfo overflow \n");
	} else {
		u = size;
	}
	memcpy(smBuf + smBufIndex, data, u);
	smBufIndex += size;
	return 0;
}

/**
 * @brief make a message with several 'float' numbers
 * 
 * @param type message type
 * @param ...  all the float numbers
 * @return message size in bytes
 */
int sendFmessage(unsigned char type, int n, ...) {
	va_list args;
	va_start(args, n);
	float* ftmp = new float[n];
	for (int i = 0; i < n; i++) 	{
		ftmp[i] = va_arg (args, double);
	}
	int u = n * sizeof(float);
	sendMsg((const unsigned char*)ftmp, type, u);
	va_end(args);
	delete[] ftmp;
	return u;
}

// -----------------------------------------------------------------------------------------------------------------------

static std::mutex xmpMutex;
static const int sbSize = 1024;
static char sbuf[sbSize];


int xmprintf(int q, const char * s, ...) {

	if (q > currentLogLevel) {
		return 0;
	}
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
