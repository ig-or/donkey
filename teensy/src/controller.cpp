

#include "controller.h"
#include "receiver.h"
#include "motors.h"
#include "teetools.h"
#include "sr04.h"

#include "wiring.h"
#include <cstring>

struct RcvInfo {
	int v = 0;
	unsigned int timeMS = 0;
};

RcvInfo rcvInfoRcv[rcc];
RcvInfo rcvInfoCopy[rcc];

enum RcvChannelState {
	chUnknown,
	chYes,  //  working
	chNo	// not working
};
RcvChannelState chState[rcc] = {chUnknown, chUnknown};

enum MotorControlState {
	mcUnknown,
	mcStop,
	mcForward,
	mcBackward
};
MotorControlState mcState = mcUnknown;

// transmitter steering callback
void rcv_ch1(int v) {
	rcvInfoRcv[0].v = v;
	rcvInfoRcv[0].timeMS = millis();
	//steering(val);
	//xmprintf(3, "CH1\t v = %d; val = %d  \r\n", v, val);
}

/** transmitter gas callback
 *  \param v from 0 to 180
 */
void rcv_ch2(int v) {
	rcvInfoRcv[1].v = v;
	rcvInfoRcv[1].timeMS = millis();
	//moveTheVehicle(val);
	//xmprintf(3, "CH2\t v = %d; val = %d  \r\n", v, val);
}


void controlSetup() {
	mcState = mcUnknown;
	setReceiverUpdateCallback(rcv_ch1, 1);
	setReceiverUpdateCallback(rcv_ch2, 2);
}

/**
 * copy info from inside receiver interrups space and 
 * also decide if receiver/transmitter channels are workign or not.
 * 
 * put info into chState and rcvInfoCopy
*/
void processReceiverState(unsigned int now) {
	int i;
	bool irq = disableInterrupts();
	memcpy(rcvInfoCopy, rcvInfoRcv, sizeof(RcvInfo) * 2);
	enableInterrupts(irq);
	RcvChannelState chStateCopy[rcc];
	for (i = 0; i < rcc; i++) {
		if ((now <= rcvInfoCopy[i].timeMS) || ((now - rcvInfoCopy[i].timeMS) < 100)) {
			chStateCopy[i] = chYes;
		} else {
			chStateCopy[i] = chNo;
		}
		if (chStateCopy[i] != chState[i]) {
			chState[i] = chStateCopy[i];
			xmprintf(3, "receiver channel %d %s \r\n", i, chState[i] == chYes ? "working" : "stopped");
		}
	}
}

/**
 * \param a the speed, from 0 to 180.  90 is stop.
*/
MotorControlState getCurrentMcState(int a) {
	MotorControlState state;

	if (a < 90 - 10) {
		state =  mcForward;
	} else if (a > 90 + 10) {
		state = mcBackward;
	} else {
		state = mcStop;
	}
	return state;
}

bool veloLimit = false;
static unsigned char wdCounter = 0;
/**
 * 
 * \param a the speed, from 0 to 180.  90 is stop.
 *  a=0 -> full forward; a = 180 => full backward;
*/
int wallDetector(int a) {
	
	MotorControlState state = getCurrentMcState(a);
	//switch the ultrasonic
	if (state != mcState) {
		switch (state) {
		case  mcForward: 
			usStartPing(0); 
			xmprintf(3, "start moving forward \r\n");
			break;
		case mcBackward: 
			usStartPing(1); 
			xmprintf(3, "start moving backward \r\n");
			break;
		default: 
			usStartPing(-1);  
			xmprintf(3, "stop \r\n");
			break;
		};
		mcState = state;
	}

	// apply the speed limit
	SR04Info us;
	int minA = 0, maxA = 0, aa = 90;
	const int maxDist = 1600; // [mm]
	switch (mcState) {
	case mcForward:
		getSR04Info(0, us);
		if ((us.quality == 1) && (us.distance_mm < maxDist)) { // have some measurement
			minA = 90 - (90 * us.distance_mm) / maxDist;
			aa = (a < minA) ? minA : a;
		} else {
			aa = a;
		}
		break;
	case mcBackward:
		getSR04Info(1, us);
		if ((us.quality == 1) && (us.distance_mm < maxDist)) { // have some measurement
			maxA = 90 + (90 * us.distance_mm) / maxDist;
			aa = (a > maxA) ? maxA : a;
		} else {
			aa = a;
		}
		break;
	default: aa = a;
	};
	if (wdCounter % 50 == 0) {
		xmprintf(3, "wallDetector mcState=%d;  a = %d;  aa = %d;  us.quality = %d;  us.distance_mm = %d; minA = %d; maxA = %d \r\n", 
		mcState, a, aa, us.quality, us.distance_mm, minA, maxA);
	}
	wdCounter += 1;
	return aa;
}





void controlFromTransmitter(unsigned int now) {
	steering(rcvInfoCopy[0].v);

	int a = rcvInfoCopy[1].v;
	int aa = wallDetector(a);
	if ((a != aa) != veloLimit) {
		veloLimit = a != aa;
		xmprintf(3, "veloLimit %s \r\n", veloLimit ? "start" : "stop");
	}
	
	moveTheVehicle(rcvInfoCopy[1].v);
	moveTheVehicle(aa);
}

static unsigned int controlCounter = 0;
void control100() {
	unsigned int now = millis();
	processReceiverState(now);
	if (chState[0] == chYes && chState[1] == chYes) {
		controlFromTransmitter(now);
	}


}


 