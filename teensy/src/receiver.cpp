



#include "receiver.h"
#include "elapsedMillis.h"
#include "ttpins.h"
#include "teetools.h"
#include "xmfilter.h"

//int delta = 2;

static const int steerMaxMks = 1787;		// right ? 
static const int steerMaxStopMks = 1560;
static const int steerZeroMks = 1547;		//  stright
static const int steerMinStopMks = 1540;
static const int steerMinMks = 1297;		//  left ?

static const int motorMaxMks = 1754;		// full backward
static const int motorMaxStopMks = 1560; 	//  backward
static const int motorZeroMks = 1550;		//  stop
static const int motorMinStopMks = 1540;	//  forward
static const int motorMinMks = 1305;      //  full gas

static const int shiftMaxMks = 1780;
static const int shiftMinMks = 1294;

enum CalibrationState {
	rcNo,
	rcGettingMiddle,
	rcMovingDown,
	rcMovingDown2,
	rcGettingDown, 
	rcMovingUp,
	rcMovingUp2,
	rcGettingUp,
	rcComplete
};

struct RCCalibrationInfo {
	int center = 0;
	int minimum = 0;
	int maximum = 0;
	void rciReset() { center = 0; minimum = 0; maximum = 0; }
};

static bool calibrationNow = false;
static unsigned int calibrationStartTimeMs = 0;
static unsigned int calibrationStateTimeMs = 0;
static unsigned int calibrationDataTimeMs = 0;
static int calibrationDataCounter = 0;
CalibrationState cState = rcNo;
RCCalibrationInfo rccInfo;
XCov1 xcov;

struct ReceiverChannel {
	int chID = 0; 
	volatile long wch = 0; //the number of microseconds between the pulses
	volatile  long cch = 0; //the number of microseconds of the pulse
	volatile  long cchPrev = 0; //the number of microseconds of the pulse
	volatile bool working = false;
	elapsedMicros chTime;
	//bool callbackEnabled = true;
	recvChangeT cb = 0;		//  callback function
	//volatile unsigned int lastSignalTimeMs = 0;
	int pin = 0;
	int deadBand = 0;

	int rangeMinMks = 0;
	int rangeMaxMks = 0;
	void rcvUpdate();
	void setupRange(int minMks, int maxMks, int zeroMks);
	void rcvPrint();
};

void ReceiverChannel::rcvUpdate() {
	if (!chID) return; //   should be not 0
   if( digitalReadFast(pin) == HIGH ) { 
		wch = chTime;
		chTime = 0;
		//lastSignalTimeMs = millis();
   }   else { 
		cch = chTime;
		if (cb != 0 &&  !calibrationNow) {
			int v = map(cch, rangeMinMks, rangeMaxMks, 0, 180);
			cb(v);

		/*
			if ((cch > (cchPrev + deadBand)) || (cch < (cchPrev - deadBand))) {
				cchPrev = cch;
				if (cch != 0) {
					if (working) {
						cb(receiverFlagOK, cch);
					} else {
						working = true;
						cb(receiverFlagControlStarted, cch);
					}
				}
			}
			*/
		}
	}
	
}

void ReceiverChannel::rcvPrint() {
	xmprintf(3, "receiver CH %d cch = %d   wch = %d \r\n", chID, cch, wch);
}

void ReceiverChannel::setupRange(int minMks, int maxMks, int zeroMks) {
	int d1 = zeroMks - minMks;
	int d2 = maxMks - zeroMks;
	int d = (d1 < d2) ? d1 : d2;

	rangeMinMks = zeroMks - d;
	rangeMaxMks = zeroMks + d;
}

ReceiverChannel ch[rcc];

void checkCH1() {
	ch[0].rcvUpdate();
}
void checkCH2() {
	ch[1].rcvUpdate();
}

/*
unsigned long rcv_ch1() {
	return cch1;
}
unsigned long rcv_ch2() {
	return cch2;
}
*/


void receiverPrint() {
	//return;
	ch[0].rcvPrint();
	ch[1].rcvPrint();
}

void receiverSetup() {
	xmprintf(1, "receiverSetup() .... ");
	//;
	ch[0].pin = rcv_ch1pin;  ch[0].chID = 1;   ch[0].deadBand = 5;
	ch[1].pin = rcv_ch2pin;	 ch[1].chID = 2;	ch[1].deadBand = 8;

	ch[0].setupRange(steerMinMks, steerMaxMks, steerZeroMks);
	ch[1].setupRange(motorMinMks, motorMaxMks, motorZeroMks);

	attachInterrupt( rcv_ch1pin, checkCH1, CHANGE );  
	attachInterrupt( rcv_ch2pin, checkCH2, CHANGE );  
	xmprintf(17, "...OK \r\n");
}

void setReceiverUpdateCallback(recvChangeT f, int chNum) {
	switch (chNum) {
	case 1: ch[0].cb = f; break;
	case 2: ch[1].cb = f; break;
	default: xmprintf(3, "setReceiverUpdateCallback error \n");
	};
}

void startReceiverCalibrate() {
	//ch[1].callbackEnabled = false;
	calibrationNow = true;
	calibrationStartTimeMs = millis();
	calibrationStateTimeMs = calibrationStartTimeMs;
	calibrationDataTimeMs = calibrationStartTimeMs;
	cState = rcGettingMiddle;
	calibrationDataCounter = 0;
	rccInfo.rciReset();
	xmprintf(3, "starting receiver calibration..  please wait \r\n");
}

static const int rcINterval = 50;

void calibrationFailed() {
	xmprintf(3, "calibration failed \r\n");
	cState = rcNo;
	calibrationNow = false;

}
void receiverProcess(unsigned int now) {
	if (!calibrationNow) {
		return;
	}
	if ((cState == rcNo) || (cState == rcComplete)) {
		return;
	}

	if ((now - calibrationDataTimeMs) < rcINterval) {
		return;
	}
	bool irq = disableInterrupts();
	int v = ch[1].cch;
	enableInterrupts(irq);

	switch (cState) {
		case rcGettingMiddle:
			rccInfo.center += v;
			calibrationDataCounter += 1;
			if (calibrationDataCounter >= 25)  {
				rccInfo.center /= calibrationDataCounter;
				cState = rcMovingDown;
				calibrationStateTimeMs = now;
				calibrationDataCounter  = 0;
				xmprintf(3, "press the gas pedal \r\n");
			}
		break;
		case rcMovingDown:
			if ((now - calibrationStateTimeMs) > 5000) {
				calibrationFailed();	
				break;		
			}
			if ((rccInfo.center - v) > 100) {
				calibrationStateTimeMs = now;
				xcov.reset();
				calibrationDataCounter  = 0;
				cState = rcMovingDown2;
				xmprintf(3, " -> rcMovingDown2 \r\n");
			}
		break;
		case rcMovingDown2:
			if ((now - calibrationStateTimeMs) > 5000) {
				calibrationFailed();	
				break;		
			}
			xcov.add(v);
			calibrationDataCounter  += 1;
			if (calibrationDataCounter >= 20) {
				float std = sqrt(xcov.cov());
				xmprintf(3, "std = %.4f \r\n");
				calibrationDataCounter  = 0;
				if (std > TWO) { //  try one more time
					xcov.reset();
					xmprintf(3,  "   not yet \r\n");
				} else {
					cState = rcGettingDown;
					calibrationStateTimeMs = now;
					xmprintf(3, " -> rcGettingDown \r\n");
				}
			}

		break;
		case rcGettingDown:
			rccInfo.minimum += v;
			calibrationDataCounter += 1;
			if (calibrationDataCounter >= 25)  {
				rccInfo.minimum /= calibrationDataCounter;
				cState = rcMovingUp;
				calibrationStateTimeMs = now;
				calibrationDataCounter  = 0;
				xmprintf(3, "press the gas pedal backward \r\n");
			}
		break;
		case rcMovingUp:
			if ((now - calibrationStateTimeMs) > 10000) {
				calibrationFailed();	
				break;		
			}
			if ((v - rccInfo.center) > 100) {
				calibrationStateTimeMs = now;
				xcov.reset();
				calibrationDataCounter  = 0;
				cState = rcMovingUp2;
				xmprintf(3, " -> rcMovingUp2 \r\n");
			}
		break;
		case rcMovingUp2:
			if ((now - calibrationStateTimeMs) > 10000) {
				calibrationFailed();	
				break;		
			}
			xcov.add(v);
			calibrationDataCounter  += 1;
			if (calibrationDataCounter >= 20) {
				float std = sqrt(xcov.cov());
				xmprintf(3, "std = %.4f \r\n");
				calibrationDataCounter  = 0;
				if (std > TWO) { //  try one more time
					xcov.reset();
					xmprintf(3,  "   not yet \r\n");
				} else {
					cState = rcGettingUp;
					calibrationStateTimeMs = now;
					xmprintf(3, " -> rcGettingUp \r\n");
				}
			}
		break;
		case rcGettingUp:
			rccInfo.maximum += v;
			calibrationDataCounter += 1;
			if (calibrationDataCounter >= 25)  {
				rccInfo.maximum /= calibrationDataCounter;
				cState = rcComplete;
				calibrationStateTimeMs = now;
				calibrationDataCounter  = 0;
				calibrationNow = false;
				xmprintf(3, "Calibration complete! data: [%d  %d  %d]\r\n", rccInfo.minimum, rccInfo.center, rccInfo.maximum);
			}
		break;
		case rcComplete:

		break;

	};


}
