

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

enum ControlMode {
	defaultControlMode,
	controlModeFromTransmitter
};
ControlMode controlMode = defaultControlMode;

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
	if (fCounter & 1) { //  make 50 HZ
		control100();
	}
}


void controlSetup() {
	xmprintf(1, "control setup ... .. ");
	mcState = mcUnknown;
	enableMotor(mmRunning_08);
	//mpf.pfInit(ca3_08_50, cb3_08_50);
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
 * put info into chState and rcvInfoCopy.
 * \param[in] now is the current time
*/
void processReceiverState(unsigned int now) {
	int i;
	//  copy receiver's values from its buffers 
	bool irq = disableInterrupts();    //  FIXME: disable only receiver interrupt here..  check that receiver interrupt is higher priority that this code
	memcpy(rcvInfoCopy, rcvInfoRcv, sizeof(RcvInfo) * 2); // this is very fast
	enableInterrupts(irq);
	RcvChannelState chStateCopy[rcc];
	for (i = 0; i < rcc; i++) {
		if ((now <= rcvInfoCopy[i].timeMS) || ((now - rcvInfoCopy[i].timeMS) < 100)) {  // we got the measurements from the receiver right now
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


const int zeroRate = 90;
const int stopRate = 19;
const int motorRange = zeroRate - stopRate;

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
	//mpf.pfNext(zeroRate);
	enableInterrupts(irq);
}


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
	SR04Info us1;
	SR04Info us2;
	SR04Info& us = us2;
	int minmax = 0, aa = 90;
	const float maxDist = 1400.0; // [mm]
	const float minDist = 120; // mm
	switch (mcState) {
	case mcForward:
		getSR04Info(0, us1);
		getSR04Info2(0, us2);
		if (us.quality == 1) {  // have some measurement
			if (us.distance_mm < minDist) { 
				aa = zeroRate;
			} else 	if (us.distance_mm < maxDist) { 
				minmax = zeroRate - stopRate - (motorRange * us.distance_mm) / maxDist;
				aa = (a < minmax) ? minmax : a;
			} else {
				aa = a;
			}
		} else {
			aa = a;
		}
		break;
	case mcBackward:
		getSR04Info(1, us1);
		getSR04Info2(1, us2);
		//us.quality = 0;
		if (us.quality == 1) {
			if (us.distance_mm < minDist) { 
				aa = zeroRate;
			} else 	if (us.distance_mm < maxDist) { // have some measurement
				minmax = zeroRate + stopRate + (motorRange * us.distance_mm) / maxDist;
				aa = (a > minmax) ? minmax : a;
				//xmprintf(17, "~");
			} else {
				aa = a;
			}
		} else {
			aa = a;
		}
		break;
	default: aa = zeroRate;
	};
	//xmprintf(17, "#");
	if (wdCounter % 50 == 0) {
		//xmprintf(3, "wallDetector mcState=%d;  a = %d;  aa = %d;  us.quality = %d;  us.distance_mm = %d; minA = %d; maxA = %d \r\n", 
		//mcState, a, aa, us.quality, us.distance_mm, minA, maxA);
	}

	//   lets run the low pass filter
	long bb;
	if (mMode == mmStop) {
		bb = std::lround(mpf.pfNext(zeroRate));
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

		info.us_distance_mm1 = us1.distance_mm;
		info.us_quality1 = us1.quality;
		info.us_timestamp1 = us1.measurementTimeMs;

		info.us_distance_mm2 = us2.distance_mm;
		info.us_quality2 = us2.quality;
		info.us_timestamp2 = us2.measurementTimeMs;

		info.mcState = mcState;

		lfSendMessage<xqm::Control100Info>(&info);

	wdCounter += 1;
	//xmprintf(17, "@");
	return bb;
}

void controlFromTransmitter(unsigned int now) {
	steering(rcvInfoCopy[0].v);

	int a = rcvInfoCopy[1].v;
	if (a < 0) { a = 0; }
	if (a > 179) { a = 179; }
	int bb = wallDetector(a, now);
	//xmprintf(17, "^");
	//if ((a != bb) != veloLimit) {
	//	veloLimit = a != bb;
		//xmprintf(3, "veloLimit %s \r\n", veloLimit ? "start" : "stop");
	//}
	
	//moveTheVehicle(rcvInfoCopy[1].v);
	moveTheVehicle(bb);
	//xmprintf(17, "&");
}

static unsigned int controlCounter = 0;
volatile char controlIsWorkingNow = false; //  FIXME: make std::atomic variable here
static bool printReceiverValues = false;

static int mSpeed = 90.0; // zeroRate;
void mSetSpeed(int s) {
	mSpeed = s;
}

/**
 *  this is the 50HZ control algorithm.  should it be fixed rate or as fast as we can?
 * 
*/
void control100() {
	//return;
	if (controlIsWorkingNow != 0) {
		return;
	}
	controlIsWorkingNow = 1;
	int bb;
	unsigned int now = millis();

	processReceiverState(now);	// copy info from inside receiver's buffers

	ControlMode cMode = defaultControlMode;
	if (chState[0] == chYes && chState[1] == chYes) {		//  transmittre is ON
		cMode = controlModeFromTransmitter;
	}
	if (controlMode != cMode) {  							//   control mode changed
		if (cMode == defaultControlMode) {					//  transmitter was OFF
			mSpeed = zeroRate;
		}
	}
	controlMode = cMode;

	switch (controlMode) {
		case defaultControlMode: 
			bb = std::lround(mpf.pfNext(mSpeed));
			moveTheVehicle(bb);
			break;
		case controlModeFromTransmitter:
			controlFromTransmitter(now);
		break;
	};

	controlIsWorkingNow = 0;
	controlCounter += 1;
	if (controlCounter  % 50 == 0) {
		if (printReceiverValues ) {
			xmprintf(3, "rcv: [%d, %d]  \r\n", rcvInfoCopy[0].v, rcvInfoCopy[1].v); //  printing receiver codes
		}
	}
	//xmprintf(17, "#");
}

void controlPrint() {
	xmprintf(3, "control100(%u)  controlFromTransmitter: %s; rcv: [%d, %d]  \r\n", 
		controlCounter, (chState[0] == chYes && chState[1] == chYes) ? "yes" : "no",
		rcvInfoCopy[0].v, rcvInfoCopy[1].v);
}
void cPrintRcvInfo(bool print) {
	printReceiverValues = print;
}


 