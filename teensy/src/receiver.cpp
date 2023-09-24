



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

enum ChCalibrationMethod {
	rcNoCalibration,
	rcCalibrationOnlyZero,
	rcFullCalibration
};
ChCalibrationMethod chCalibrationMethod = rcCalibrationOnlyZero;

struct RCCalibrationInfo {
	int center = 0;
	int minimum = 0;
	int maximum = 0;
	void rciReset() { center = 0; minimum = 0; maximum = 0; }
};

struct RCHCalibration {
	ChCalibrationMethod cMethod = rcCalibrationOnlyZero;
	unsigned int calibrationStartTimeMs = 0;
	unsigned int calibrationStateTimeMs = 0;
	unsigned int calibrationDataTimeMs = 0;
	int calibrationDataCounter = 0;
	CalibrationState cState = rcNo;
	RCCalibrationInfo rccInfo;
	XCov1 xcov;
	void startReceiverChannelCalibration(unsigned int now);
	void calibrationFailed();
	void zeroCalibrationComplete();
};

void RCHCalibration::startReceiverChannelCalibration(unsigned int now) {
	cState = rcCalibrationStarting;
	calibrationStartTimeMs = now;
	calibrationStateTimeMs = now;
	calibrationDataTimeMs = now;
	calibrationDataCounter = 0;
	rccInfo.rciReset();
	xmprintf(3, "starting receiver ch calibration..  please wait \r\n" );
}
void RCHCalibration::calibrationFailed() {
	xmprintf(3, "calibration failed \r\n");
	cState = rcNo;
}
void RCHCalibration::zeroCalibrationComplete() {
	xmprintf(3, "zero calibration completed \r\n");
	cState = rcComplete;
}

/**
 *  \struct ReceiverChannel
*/
struct ReceiverChannel {
	int chID = 0; 						//  channel ID
	volatile long wch = 0; 				// the number of microseconds between the pulses
	volatile  long cch = 0; 			// the number of microseconds of the pulse
	unsigned int prevCchTime = 0;
	volatile unsigned int cchTime = 0;	// milliseconds when we got the info
	volatile  long cchPrev = 0; 		// the number of microseconds of the previous pulse
	volatile bool working = false;
	elapsedMicros chTime;
	//bool callbackEnabled = true;
	recvChangeT cb = 0;					//  callback function
	//volatile unsigned int lastSignalTimeMs = 0;
	int pin = 0;
	int deadBand = 0;
	int rangeMinMks = 0;
	int rangeZeroMks = 0;
	int rangeMaxMks = 0;
	RCHCalibration chCalibration;
	void rcvUpdate();
	/**
	 * setup receiver changing range 
	*/
	void setupRange(int minMks, int maxMks, int zeroMks);
	/// @brief   print out receiver statistics
	void rcvPrint();					
	void processCalibration(unsigned int now);
};

void ReceiverChannel::rcvUpdate() {
	if (!chID) return; //   should be not 0
   if( digitalReadFast(pin) == HIGH ) { 
		wch = chTime;
		chTime = 0;
		//lastSignalTimeMs = millis();
   }   else { 
		cch = chTime;
		cchTime = millis();
		if ((prevCchTime == 0) || (cchTime > (prevCchTime + 1000))) { //   there was a big interval.. just turned on
			chCalibration.startReceiverChannelCalibration(cchTime);
		}
		if (chCalibration.cState == rcComplete) {
			if ((cb != 0)) {		//  only if calibrated
				int v = map(cch, rangeMinMks, rangeMaxMks, 0, 180);
				cb(v, cchTime);
			}
		} else {  //  not calibrated?
			processCalibration(cchTime);
		}
	}
	prevCchTime = cchTime;
}

void ReceiverChannel::rcvPrint() {
	xmprintf(3, "receiver CH %d cch = %d   wch = %d \r\n", chID, cch, wch);
}

void ReceiverChannel::setupRange(int minMks, int maxMks, int zeroMks) {
	rangeZeroMks = zeroMks;
	int d1 = zeroMks - minMks;
	int d2 = maxMks - zeroMks;
	int d = (d1 < d2) ? d2 : d1;

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
	//ch[1].chCalibration.startReceiverChannelCalibration();
}

static const int rcInterval = 50;

void calibrationFailed() {
	ReceiverChannel& rc = ch[1]; //  motor channel
	rc.chCalibration.calibrationFailed();
}

void ReceiverChannel::processCalibration(unsigned int now) {
	CalibrationState& cState = chCalibration.cState;
	if ((cState == rcNo) || (cState == rcComplete)) {
		return;
	}
	RCHCalibration& c = chCalibration;

	if ((now - c.calibrationDataTimeMs) < rcInterval) { 	// not so fast
		return;
	}
	bool irq = disableInterrupts();							//  get the receiver values
	int v = cch;
	enableInterrupts(irq);

	switch (cState) {
		case rcCalibrationStarting:							//   just wait for some time
			c.calibrationDataCounter += 1;
			if (c.calibrationDataCounter >= 10) {
				c.calibrationDataCounter  = 0;
				cState = rcGettingMiddle;
			}
			break;
		case rcGettingMiddle:
			c.rccInfo.center += v;
			c.calibrationDataCounter += 1;
			if (c.calibrationDataCounter >= 25)  {
				c.rccInfo.center /= c.calibrationDataCounter;
				if (c.cMethod == rcCalibrationOnlyZero) {
					c.zeroCalibrationComplete();
					setupRange(rangeMinMks, rangeMaxMks, c.rccInfo.center);
					break;
				} else {
					cState = rcMovingDown;
					c.calibrationStateTimeMs = now;
					c.calibrationDataCounter  = 0;
					xmprintf(3, "press the trottle (full forward speed) \r\n");
				}
			}
			break;
		case rcMovingDown:
			if ((now - c.calibrationStateTimeMs) > 5000) {
				calibrationFailed();	
				break;		
			}
			if ((c.rccInfo.center - v) > 100) {
				c.calibrationStateTimeMs = now;
				c.xcov.reset();
				c.calibrationDataCounter  = 0;
				cState = rcMovingDown2;
				xmprintf(3, " -> rcMovingDown2 \r\n");
			}
			break;
		case rcMovingDown2:
			if ((now - c.calibrationStateTimeMs) > 5000) {
				calibrationFailed();	
				break;		
			}
			c.xcov.add(v);
			c.calibrationDataCounter  += 1;
			if (c.calibrationDataCounter >= 20) {
				float std = sqrt(c.xcov.cov());
				xmprintf(3, "std = %.4f \r\n");
				c.calibrationDataCounter  = 0;
				if (std > TWO) { //  try one more time
					c.xcov.reset();
					xmprintf(3,  "   not yet \r\n");
				} else {
					cState = rcGettingDown;
					c.calibrationStateTimeMs = now;
					xmprintf(3, " -> rcGettingDown \r\n");
				}
			}
		break;
		case rcGettingDown:
			c.rccInfo.minimum += v;
			c.calibrationDataCounter += 1;
			if (c.calibrationDataCounter >= 25)  {
				c.rccInfo.minimum /= c.calibrationDataCounter;
				cState = rcMovingUp;
				c.calibrationStateTimeMs = now;
				c.calibrationDataCounter  = 0;
				xmprintf(3, "press the gas pedal backward \r\n");
			}
		break;
		case rcMovingUp:
			if ((now - c.calibrationStateTimeMs) > 10000) {
				calibrationFailed();	
				break;		
			}
			if ((v - c.rccInfo.center) > 100) {
				c.calibrationStateTimeMs = now;
				c.xcov.reset();
				c.calibrationDataCounter  = 0;
				cState = rcMovingUp2;
				xmprintf(3, " -> rcMovingUp2 \r\n");
			}
		break;
		case rcMovingUp2:
			if ((now - c.calibrationStateTimeMs) > 10000) {
				calibrationFailed();	
				break;		
			}
			c.xcov.add(v);
			c.calibrationDataCounter  += 1;
			if (c.calibrationDataCounter >= 20) {
				float std = sqrt(c.xcov.cov());
				xmprintf(3, "std = %.4f \r\n");
				c.calibrationDataCounter  = 0;
				if (std > TWO) { //  try one more time
					c.xcov.reset();
					xmprintf(3,  "   not yet \r\n");
				} else {
					cState = rcGettingUp;
					c.calibrationStateTimeMs = now;
					xmprintf(3, " -> rcGettingUp \r\n");
				}
			}
			break;
		case rcGettingUp:
			c.rccInfo.maximum += v;
			c.calibrationDataCounter += 1;
			if (c.calibrationDataCounter >= 25)  {
				c.rccInfo.maximum /= c.calibrationDataCounter;
				cState = rcComplete;
				c.calibrationStateTimeMs = now;
				c.calibrationDataCounter  = 0;
				cState = rcComplete;
				setupRange(c.rccInfo.minimum, c.rccInfo.maximum, c.rccInfo.center);
				xmprintf(3, "Calibration complete! data: [%d  %d  %d]\r\n", c.rccInfo.minimum, c.rccInfo.center, c.rccInfo.maximum);
			}
		break;
		case rcComplete:

			break;
	};
}



