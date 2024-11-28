

#include "eye.h"
#include "eth_api.h"
#include "xmessagesend.h"

int xmprintf(int q, const char * s, ...);

Eye::Eye() {

}
Eye::~Eye() {

}
void Eye::slScan(SLPoint* p, int n) {

}
void Eye::slFrontObstacle(float distance) {
	MFrontObstacle m;
	m.distance = distance;
	sendMsg(&m);
	if (sfCounter % 2 == 0) {
		xmprintf(7, "distance = %.3f\n", distance);
	}

	sfCounter += 1;
}

/// from eth thread
void Eye::ethData(char* s, int size) {

}

/// from eth thread
void Eye::ethPing(unsigned char, unsigned int) {

}

void Eye::startEye() {
	if (st.joinable()) { //  working already?
		return;
	}
	pleaseStop = false;
	st = std::thread(&Eye::see, this);
}
void Eye::stopEye() {
	xmprintf(6, "stopping eye ...  \n");
	pleaseStop = true;
	if (!st.joinable()) { //  already stopped?
		return;
	}
	st.join();
	xmprintf(6, "eye finished  \n");
}
void Eye::see() {
	while (!pleaseStop) {
		std::this_thread::yield();

	}
}
