

#include "eye.h"

int xmprintf(int q, const char * s, ...);

Eye::Eye() {

}
Eye::~Eye() {

}
void Eye::slScan(SLPoint* p, int n) {

}
void Eye::slFrontObstacle(float distance) {
	if (sfCounter % 10 == 0) {
		xmprintf(6, "distance = %.3f\n", distance);
	}

	sfCounter += 1;
}
void Eye::ethData(char* s, int size) {

}
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
	pleaseStop = true;
	if (!st.joinable()) { //  already stopped?
		return;
	}
	st.join();
}
void Eye::see() {
	while (!pleaseStop) {
		std::this_thread::yield();

	}
}
