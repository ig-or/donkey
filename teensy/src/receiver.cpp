



#include "receiver.h"
#include "elapsedMillis.h"
#include "ttpins.h"
#include "teetools.h"
#include "xmfilter.h"
#include "led_strip.h"

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

//ChCalibrationMethod chCalibrationMethod = rcCalibrationOnlyZero;



void RCHCalibration::startReceiverChannelCalibration(unsigned int now) {
	cState = rcCalibrationStarting;
	calibrationStartTimeMs = now;
	calibrationStateTimeMs = now;
	calibrationDataTimeMs = now;
	calibrationDataCounter = 0;
	rccInfo.rciReset();
	
	//xmprintf(3, "starting receiver ch calibration..  please wait \r\n" );
}
void RCHCalibration::calibrationFailed() {
	ledstripMode(lsManualControl, 128, 0x00750000);
	xmprintf(3, "calibration failed \r\n");
	cState = rcNo;
}
void RCHCalibration::zeroCalibrationComplete() {
	//ledstripMode(lsManualControl, 128, 0x00007500);
//	xmprintf(3, "zero calibration completed \r\n");
	cState = rcComplete;
}

/**
 *  \struct ReceiverChannel
*/
struct ReceiverChannel {
	volatile long wch = 0; 				// the number of microseconds between the pulses
	volatile  long cch = 0; 			// the number of microseconds of the pulse
	volatile unsigned int cchTime = 0xffffffff;	// milliseconds when we got the info

	elapsedMicros chTime;
	int pin = 0;
	int chID = 0; 						//  channel ID
	
	void rcvUpdate();					//  called from input pin interrupt
};

void RcvInfo::rchUpdate(unsigned int now) {
	switch (chState) {
	case 	chUnknown: 
		if (cchTime == 0xffffffff) {	//  not yet
			break;
		}
		if (cchTime + 500 > now) {
			chState = chCalibration;
			calibration.startReceiverChannelCalibration(cchTime);
			ledstripMode(lsManualControl, 128, 0x00000075);
			xmprintf(3, "RcvInfo::rchUpdate()   receiver channel %d started calibration \r\n", chID);
		}
		break;
	case 	chCalibration:
		if (cchTime + 400 < now) {
			chState = chUnknown;
			xmprintf(3, "RcvInfo::rchUpdate()   receiver channel %d stopped (calibration failed)\r\n", chID);
			break;
		}
		processCalibration(cchTime);
		break;
	case 	chYes:  //  working
		v = map(cch, rangeMinMks, rangeMaxMks, 0, 180);
		if (cchTime + 400 < now) {
			chState = chNo;
			xmprintf(3, "RcvInfo::rchUpdate()   receiver channel %d stopped \r\n", chID);
		}
		break;
	case 	chNo:	// not working
		if (cchTime + 500 > now) {

			chState = chYes;
			xmprintf(3, "RcvInfo::rchUpdate()   receiver channel %d started \r\n", chID);
		}
		break;
	};

	prevCchTime = cchTime;
}

RcvInfo ri[rcc];

void ReceiverChannel::rcvUpdate() {
	//if (!chID) return; //   should be not 0
	if( digitalReadFast(pin) == HIGH ) { 
		wch = chTime;
		chTime = 0;
	}   else { 
		cch = chTime;
		cchTime = millis();
	}
}

void RcvInfo::rcvPrint() {
	xmprintf(3, "receiver CH %d cch = %d   wch = %d \r\n", chID, cch, wch);
}

void RcvInfo::setupRange(int minMks, int maxMks, int zeroMks) {
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

void rcvProcess(unsigned int now) {
	int i;
	//bool irq = disableInterrupts();									//  copy the info from rcv interrupts into the local buffers
	NVIC_DISABLE_IRQ(IRQ_GPIO6789);
	for (i = 0; i < rcc; i++) {
		ri[i].cch = ch[i].cch;
		ri[i].wch = ch[i].wch;
		ri[i].cchTime = ch[i].chTime;
	}
	//enableInterrupts(irq);
	NVIC_ENABLE_IRQ(IRQ_GPIO6789);

	for (i = 0; i < rcc; i++) {										// do some processing realted to receiver channels
		ri[i].rchUpdate(now);
	}
}

void receiverPrint() {
	//return;
	ri[0].rcvPrint();
	ri[1].rcvPrint();
}

void receiverSetup() {
	xmprintf(1, "receiverSetup() .... ");
	
	ch[0].pin = rcv_ch1pin;  ch[0].chID = 1;   	
	ch[1].pin = rcv_ch2pin;	 ch[1].chID = 2;	

	ri[0].chID = 1; ri[0].deadBand = 5;
	ri[1].chID = 2; ri[1].deadBand = 8;

	ri[0].setupRange(steerMinMks, steerMaxMks, steerZeroMks);
	ri[1].setupRange(motorMinMks, motorMaxMks, motorZeroMks);

	pinMode(rcv_ch1pin, INPUT_PULLDOWN); 
	pinMode(rcv_ch2pin, INPUT_PULLDOWN); 

	attachInterrupt(rcv_ch1pin, checkCH1, CHANGE );  
	attachInterrupt(rcv_ch2pin, checkCH2, CHANGE );  
	ledstripMode(lsManualControl, 128, 0x00000008);
	xmprintf(17, "...OK \r\n");
}

static const int rcInterval = 50;

void calibrationFailed() {
	RcvInfo& i = ri[1]; //  motor channel
	i.calibration.calibrationFailed();
}

void RcvInfo::processCalibration(unsigned int now) {
	CalibrationState& cState = calibration.cState;
	if ((cState == rcNo) || (cState == rcComplete)) {
		return;
	}
	RCHCalibration& c = calibration;

	if ((now - c.calibrationDataTimeMs) < rcInterval) { 	// not so fast
		return;
	}
	int vv = cch;

	switch (cState) {
		case rcCalibrationStarting:							//   just wait for some time
			c.calibrationDataCounter += 1;
			if (c.calibrationDataCounter >= 10) {
				c.calibrationDataCounter  = 0;
				cState = rcGettingMiddle;
			}
			break;
		case rcGettingMiddle:
			c.rccInfo.center += vv;
			c.calibrationDataCounter += 1;
			if (c.calibrationDataCounter >= 25)  {
				c.rccInfo.center /= c.calibrationDataCounter;
				if (c.cMethod == rcCalibrationOnlyZero) {
					c.zeroCalibrationComplete();
					setupRange(rangeMinMks, rangeMaxMks, c.rccInfo.center);
					chState = chYes;
					ledstripMode(lsManualControl, 128, 0x00007500);
					xmprintf(3, "zero calibration ch %d completed [%d - %d - %d]\r\n", chID, rangeMinMks, rangeZeroMks, rangeMaxMks);
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
			if ((c.rccInfo.center - vv) > 100) {
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
			c.xcov.add(vv);
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
			c.rccInfo.minimum += vv;
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
			if ((vv - c.rccInfo.center) > 100) {
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
			c.xcov.add(vv);
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
			c.rccInfo.maximum += vv;
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



