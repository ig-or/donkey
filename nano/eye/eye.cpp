

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

void Eye::onString(char* s) {
	xmprintf(4, "Eye::onString(%s)", s);
	if (eth) {
		eth->do_write(s);
	}
}

/// from eth thread
void Eye::ethData(char* s, int size) {

	s[size-1] = 0; // only for printf
	xmprintf(3, "eye: %s\n", s);
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
