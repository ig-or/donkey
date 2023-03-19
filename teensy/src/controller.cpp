

#include "controller.h"
#include "receiver.h"
#include "motors.h"
#include "teetools.h"
#include "sr04.h"
#include "xmessage.h"
#include "logfile.h"
#include "xmfilter.h"


#include "wiring.h"
#include "IntervalTimer.h"


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

static IntervalTimer intervalTimer;

enum MotorMode {
	mmStop = 0, 
	mmRunning_027,  // low pass filter 0.27 Hz
	mmRunning_05, 	 // low pass filter 0.5 Hz
	mmRunning_08, 	 // low pass filter 0.8 Hz
	mmRunning_25,   // low pass filter 2.5 Hz

	mmShiftTest
};

PolyFilter<3> mpf;
static MotorMode mMode = mmStop;

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

volatile unsigned int fCounter = 0;
void control100();
void intervalFunction() {
	if (fCounter % 200 == 0) {
		//xmprintf(3, "%u intervalFunction \r\n", fCounter);
	}
	fCounter += 1;
	if (fCounter & 1) {
		control100();
	}
}


void controlSetup() {
	xmprintf(1, "control setup ... .. ");
	mcState = mcUnknown;
	enableMotor(mmStop);
	mpf.pfInit(ca3_08_50, cb3_08_50);
	setReceiverUpdateCallback(rcv_ch1, 1);
	setReceiverUpdateCallback(rcv_ch2, 2);

	intervalTimer.priority(255);
	intervalTimer.begin(intervalFunction, 10000);
	xmprintf(17, "... .. OK \r\n");
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

void enableMotor(int u) {
	bool irq = disableInterrupts();
	mMode = static_cast<MotorMode>(u);
	switch (mMode) {
		case mmRunning_027: mpf.pfInit(ca3_027_50, cb3_027_50);	 break; 	//  cut off 0.27 Hz and sampling rate = 50 HZ
		case mmRunning_05: 	mpf.pfInit(ca3_05_50, cb3_05_50);	 break; 	//  cut off 0.5 Hz and sampling rate = 50 HZ
		case mmRunning_08: 	mpf.pfInit(ca3_08_50, cb3_08_50);	 break; 	//  cut off 0.8 Hz and sampling rate = 50 HZ
		case mmRunning_25: 	mpf.pfInit(ca3_25_50, cb3_25_50);	 break; 	//  cut off 2.5 Hz and sampling rate = 50 HZ
		case mmShiftTest:   mpf.pfInit(ca3_08_50, cb3_08_50);	 break; 	
	};
	enableInterrupts(irq);
}

const int zeroRate = 90;
const int stopRate = 20;
const int motorRange = zeroRate - stopRate;

/**
 * \param a the speed, from 0 to 180.  90 is stop.
*/
MotorControlState getCurrentMcState(int a) {
	MotorControlState state;

	if (a < zeroRate - stopRate) {
		state =  mcForward;
	} else if (a > zeroRate + stopRate) {
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
int wallDetector(int a, unsigned int timestamp) {
	
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
			usStartPing(0);  
			xmprintf(3, "stop \r\n");
			break;
		};
		mcState = state;
	}

	// apply the speed limit
	SR04Info us;
	int minmax = 0, aa = 90;
	const int maxDist = 1400; // [mm]
	switch (mcState) {
	case mcForward:
		getSR04Info(0, us);
		if ((us.quality == 1) && (us.distance_mm < maxDist)) { // have some measurement
			minmax = zeroRate - stopRate - (motorRange * us.distance_mm) / maxDist;
			aa = (a < minmax) ? minmax : a;
		} else {
			aa = a;
		}
		break;
	case mcBackward:
		getSR04Info(1, us);
		if ((us.quality == 1) && (us.distance_mm < maxDist)) { // have some measurement
			minmax = zeroRate + stopRate + (motorRange * us.distance_mm) / maxDist;
			aa = (a > minmax) ? minmax : a;
		} else {
			aa = a;
		}
		break;
	default: aa = a;
	};
	if (wdCounter % 50 == 0) {
		//xmprintf(3, "wallDetector mcState=%d;  a = %d;  aa = %d;  us.quality = %d;  us.distance_mm = %d; minA = %d; maxA = %d \r\n", 
		//mcState, a, aa, us.quality, us.distance_mm, minA, maxA);
	}

	//   lets run the low pass filter
	long bb;
	if (mMode == mmStop) {
		bb = std::lround(mpf.pfNext(ZERO));
	} else {
		bb = std::lround(mpf.pfNext(aa));
	}

		xqm::Control100Info info;
		info.a = a;
		info.aa = aa;
		info.bb = bb;
		info.timestamp = timestamp;
		info.mcState = static_cast<char>(mcState);
		info.minmax = minmax;
		info.us_distance_mm = us.distance_mm;
		info.us_quality = us.quality;
		info.us_timestamp = us.measurementTimeMs;

		lfSendMessage<xqm::Control100Info>(&info);

	wdCounter += 1;
	return bb;
}

void controlFromTransmitter(unsigned int now) {
	steering(rcvInfoCopy[0].v);

	int a = rcvInfoCopy[1].v;
	int aa = wallDetector(a, now);
	if ((a != aa) != veloLimit) {
		veloLimit = a != aa;
		xmprintf(3, "veloLimit %s \r\n", veloLimit ? "start" : "stop");
	}
	
	//moveTheVehicle(rcvInfoCopy[1].v);
	moveTheVehicle(aa);
}

static unsigned int controlCounter = 0;
volatile char controlIsWorkingNow = false;
void control100() {
	//return;
	if (controlIsWorkingNow != 0) {
		return;
	}
	controlIsWorkingNow = 1;
	unsigned int now = millis();

	processReceiverState(now);
	if (chState[0] == chYes && chState[1] == chYes) {
		controlFromTransmitter(now);
	}
	
	controlIsWorkingNow = 0;
	controlCounter += 1;
}

void controlPrint() {
	xmprintf(3, "control100(%u)   \r\n", controlCounter);
}


 