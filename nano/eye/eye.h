#pragma once

#include <thread>
#include "lidar.h"
#include "eth_client.h"

class Eye : public SLidarConsumer, public EthConsumer {
public:
	Eye();
	~Eye();
	void setEthClient(EthClient* eth_) {eth = eth_; }
	void startEye();
	void stopEye();
	void onString(char* s);
private:
	EthClient* eth = 0;
	bool pleaseStop = false;
	std::thread st;
	unsigned int sfCounter = 0;

	void slScan(SLPoint* p, int n); 
	void slFrontObstacle(float distance);
	void ethData(char* s, int size);
	void ethPing(unsigned char, unsigned int);
	void see();

};