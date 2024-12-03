

#include "controller.h"
#include "receiver.h"
#include "motors.h"
#include "teetools.h"
#include "sr04.h"
#include "xmessage.h"
#include "logfile.h"
#include "xmfilter.h"
#include "led_strip.h"

#include "wiring.h"
#include "IntervalTimer.h"


#include <cstring>
#include <atomic>

enum ControlMode {
	defaultControlMode,
	controlModeFromTransmitter
};
ControlMode controlMode = defaultControlMode;

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

void onMotorStateChaged(MotorControlState mc) {
	//switch the ultrasonic
	unsigned char ls0 = 0; 
	unsigned char lsFwd = 0; 
	unsigned char lsBwd = 0; 
	switch (mc) {
	case  mcForward: 
		usStartPing(0); 
		lsFwd = 250;
		xmprintf(3, "start moving forward (forward us ping enabled)\r\n");
		break;
	case mcBackward: 
		usStartPing(1); 
		lsBwd = 250;
		xmprintf(3, "start moving backward (backward us ping enabled) \r\n");
		break;
	default: 
		usStartPing(0);  
		ls0 = 250;
		xmprintf(3, "stop.  (forward us ping enabled) \r\n");
		break;
	};

	ledstripMode(lsStop, ls0);
	ledstripMode(lsMovingBackward, lsBwd);
	ledstripMode(lsMovingForward, lsFwd);
}

void controlSetup() {
	xmprintf(1, "control setup ... .. ");
	enableMotor(mmRunning_08);
	//mpf.pfInit(ca3_08_50, cb3_08_50);

	intervalTimer.priority(255);
	intervalTimer.begin(intervalFunction, 10000);

	setupMotorStateChangedCallback(onMotorStateChaged);
	xmprintf(17, "... .. OK \r\n");
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
	//mpf.pfNext(zeroRate);
	enableInterrupts(irq);
}

bool veloLimit = false;
static unsigned char wdCounter = 0;

/**
 * 
 * \param a the speed, from 0 to 180.  90 is stop.  
 *  a=0 -> full forward; a = 180 => full backward;  
*/
int wallDetector(int a, unsigned int timestamp) {
	// apply the speed limit
	MotorControlState realMotorState = getCurrentMotorState();
	//  detect desired motor state
	MotorControlState mcState = mcUnknown;  	//  mcState is different from realMotorState
	if (a < motorZeroRate - motorStopDB) {
		mcState = mcForward;
	} else if (a < motorZeroRate + motorStopDB) {
		mcState = mcStop;
	} else {
		mcState = mcForward;
	}

	SR04Info us1;
	SR04Info us2;
	SR04Info& us = us2;
	int minmax = 0, aa = 90;
	const float maxDist = 1400.0; // [mm]
	const float minDist = 120; // mm
	const float distRange =  maxDist - minDist;
	int fObs = 0;	//  for led strip
	int rObs = 0;	//  for led strip
	switch (mcState) {
	case mcForward:
		getSR04Info(0, us1);				// get the data from forward  us sensor
		getSR04Info2(0, us2);				// get the filtered data
		if (us.quality == 1) {  			// we do have some measurement
			if (us.distance_mm < minDist) { //  too close, we should stop
				aa = motorZeroRate;
			} else 	if (us.distance_mm < maxDist) { 
				minmax = motorZeroRate - motorStopDB - (motorRange * (us.distance_mm - minDist)) / distRange;
				if (minmax > motorZeroRate) {
					minmax = motorZeroRate;
				}
				aa = (a < minmax) ? minmax : a;
				fObs = 205 - (205 * (us.distance_mm - minDist)) / distRange;
				clamp(fObs, 0, 255);
			} else {						// too far away, do not limit the control value
				aa = a;
			}
		} else {							//  no good measurements, do nothing here
			aa = a;
		}
		break;

	case mcBackward:
		getSR04Info(1, us1);						//  take some data from backward us sensor
		getSR04Info2(1, us2);
		//us.quality = 0;
		if (us.quality == 1) {						//  have some measurement
			if (us.distance_mm < minDist) { 		//  too close
				aa = motorZeroRate;
			} else 	if (us.distance_mm < maxDist) { // have some measurement
				minmax = motorZeroRate + motorStopDB + (motorRange * (us.distance_mm - minDist)) / distRange;
				aa = (a > minmax) ? minmax : a;
				//xmprintf(17, "~");
				rObs = 205 - (205 * (us.distance_mm - minDist)) / distRange;
				clamp(rObs, 0, 255);
			} else {								// too far away
				aa = a;
			}
		} else {									//  no relevant info from the sensor
			aa = a;
		}
		break;
	default: aa = motorZeroRate;
	};
	//xmprintf(17, "#");
	if (wdCounter % 50 == 0) {
		//xmprintf(3, "wallDetector mcState=%d;  a = %d;  aa = %d;  us.quality = %d;  us.distance_mm = %d; minmax = %d; rObs=%d fObs=%d\r\n", 
		//	mcState, a, aa, us.quality, us.distance_mm, minmax, rObs, fObs);
	}
	ledstripMode(lsFrontObstacle, fObs);
	ledstripMode(lsRearObstakle, rObs);

	//   lets run the low pass filter
	long bb;
	if (mMode == mmStop) {
		bb = std::lround(mpf.pfNext(motorZeroRate));
	} else {
		bb = std::lround(mpf.pfNext(aa));
	}

/*
		xqm::Control100Info info;
		info.a = a;
		info.aa = aa;
		info.bb = bb;
		info.timestamp = timestamp;
		info.mcRealState = static_cast<char>(realMotorState);
		info.mcDesiredState = static_cast<char>(mcState);
		info.minmax = minmax;

		info.us_distance_mm1 = us1.distance_mm;
		info.us_quality1 = us1.quality;
		info.us_timestamp1 = us1.measurementTimeMs;

		info.us_distance_mm2 = us2.distance_mm;
		info.us_quality2 = us2.quality;
		info.us_timestamp2 = us2.measurementTimeMs;

		lfSendMessage<xqm::Control100Info>(&info);
*/
	wdCounter += 1;
	//xmprintf(17, "@");
	return bb;
}

void controlFromTransmitter(unsigned int now) {
	xqm::XQMData8 cInfo;
	cInfo.id = 1;
	cInfo.timestamp = now;
	cInfo.data[0] = ri[0].cch;
	cInfo.data[1] = ri[0].v;

	steering(ri[0].v);				// steering is on the first channel

	int a = ri[1].v;		//  throttle is on the second channel
	if (a < 0) { a = 0; }
	if (a > 179) { a = 179; }
	int bb = wallDetector(a, now);
	//xmprintf(17, "^");
	//if ((a != bb) != veloLimit) {
	//	veloLimit = a != bb;
		//xmprintf(3, "veloLimit %s \r\n", veloLimit ? "start" : "stop");
	//}
	
	//moveTheVehicle(rcvInfoCopy[1].v);
	cInfo.data[7] = bb;
	cInfo.data[6] = a;
	cInfo.data[5] = ri[1].v;
	cInfo.data[4] = ri[1].cch;
	moveTheVehicle(bb);				//  send bb code (0 ~ 180) to the main motor
	lfSendMessage<xqm::XQMData8>(&cInfo);
	//xmprintf(17, "&");
}

static unsigned int controlCounter = 0;
std::atomic<char> controlIsWorkingNow = 0; 
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

	rcvProcess(now);							// copy info from inside receiver's buffers
	

	ControlMode cMode = defaultControlMode;
	if (ri[0].chState == chYes && ri[1].chState == chYes) {		//  transmittre is ON
		cMode = controlModeFromTransmitter;
	}
	if (controlMode != cMode) {  							//   control mode changed
		if (cMode == defaultControlMode) {					//  transmitter was OFF
			mSpeed = motorZeroRate;
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

	
	controlCounter += 1;
	if (controlCounter  % 50 == 0) {
		if (printReceiverValues ) {
			xmprintf(3, "rcv: [%d, %d]  \r\n", ri[0].v, ri[1].v); //  printing receiver codes
		}
	}
	controlIsWorkingNow = 0;
	//xmprintf(17, "#");
}

void controlPrint() {
	xmprintf(3, "control100(%u)  controlFromTransmitter: %s; rcv: [%d, %d]  \r\n", 
		controlCounter, (ri[0].chState == chYes && ri[1].chState == chYes) ? "yes" : "no",
		ri[0].v, ri[1].v);
}
void cPrintRcvInfo(bool print) {
	printReceiverValues = print;
}


 