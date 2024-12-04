



#include "receiver.h"
#include "elapsedMillis.h"
#include "ttpins.h"
#include "teetools.h"
#include "xmfilter.h"
#include "xmessage.h"
#include "logfile.h"
#include "led_strip.h"

//int delta = 2;

static const int steerMaxMks = 1787;		// right ? 
static const int steerMaxStopMks = 1560;
static const int steerZeroMks = 1547;		//  stright
static const int steerMinStopMks = 1540;
static const int steerMinMks = 1297;		//  left ?

static const int motorMaxMks = 1760;		// full backward
static const int motorMaxStopMks = 1560; 	//  backward
static const int motorZeroMks = 1550;		//  stop
static const int motorMinStopMks = 1540;	//  forward
static const int motorMinMks = 1186;    //1305;      //  full gas

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
	volatile long period = 0;
	volatile long rp, rpMin, rpMax;		// (good) periods between pulses
	volatile long rv, rvMin, rvMax;
	volatile long cch = 0; 			// the number of microseconds of the pulse
	volatile long cchCopy = 0; 			// the number of microseconds of the pulse
	volatile unsigned int cchTime = 0xffffffff;	// milliseconds when we got the info
	volatile unsigned char rUpdate = 0;
	volatile char outlierFlag1 = 0;
	volatile char outlierFlag2 = 0;
	volatile char outlierCounter = 0;

	elapsedMicros chTime;
	elapsedMicros chPeriod;
	int pin = 0;
	int chID = 0; 						//  channel ID
	char resetCounter = 3;

	int rangeMinMks = 0;
	int rangeZeroMks = 0;
	int rangeMaxMks = 0;
	
	void rcvUpdate();					//  called from input pin interrupt
	void smallReset();
};

void ReceiverChannel::smallReset() {
	NVIC_DISABLE_IRQ(IRQ_GPIO6789);
	rUpdate = 0;
	resetCounter = 3;
	cchTime = 0xffffffff;
	chTime = 0;
	chPeriod = 0;
	outlierFlag1 = 0;
	outlierCounter = 0;
	NVIC_ENABLE_IRQ(IRQ_GPIO6789);
}
ReceiverChannel ch[rcc];

void RcvInfo::rchUpdate(unsigned int now) {
	RcvChannelState s = chState;
	int q = chID - 1;
	float vf;
	int i;
	switch (chState) {
	case 	chUnknown: 
		if (cchTime == 0xffffffff || (rUpdate == 0)) {	//  not yet
			break;
		}
		if (cchTime + 500 > now) {
			chState = chCalibration;
			calibration.startReceiverChannelCalibration(cchTime);
			ledstripMode(lsManualControl, 128, 0x00000075);
			xmprintf(3, "RcvInfo::rchUpdate()   receiver channel %d started calibration [%d -> %d]\r\n", chID, s, chState);
		}
		break;

	case 	chCalibration:
		if (cchTime + 400 < now && (rUpdate == 0)) {
			chState = chUnknown;
			ch[0].smallReset();
			ch[1].smallReset();
			xmprintf(3, "RcvInfo::rchUpdate()   receiver channel %d stopped (calibration failed)  [ %d -> %d] \r\n", chID, s, chState);
			break;
		}
		if (rUpdate != 0) {
			processCalibration(cchTime);
		}
		break;

	case 	chYes:  //  working
		fcch = cch;
		vf = map(fcch, ch[q].rangeMinMks, ch[q].rangeMaxMks, 0.0f, 180.0f);
		if (vf < 0.0f) { vf = 0.0f;}
		if (vf > 180.0f) { vf = 180.0f; }

		//  update vh
		for (i = 0; i < (vhSize - 1); i++) {
			vh[i] = vh[i+1];
		}
		vh[vhSize - 1] = vf;
		memcpy(vhCopy, vh, sizeof(float) * vhSize);
		
		vf = opt_med5(vhCopy);
		//xmprintf(17, "vhCopy%d: [%.1f, %.1f, %.1f];    vf = %.1f  \r\n", chID, vhCopy[0], vhCopy[1], vhCopy[2], vf);
		v = vf;
		
		if (cchTime + 400 < now) {
			chState = chNo;
			ch[0].smallReset();
			ch[1].smallReset();

			xmprintf(3, "RcvInfo::rchUpdate()   receiver channel %d stopped  [ %d -> %d] (now = %u; cchTime = %u, rUpdate = %d)\r\n", chID, s, chState, now, cchTime, rUpdate);
		}
		break;
	case 	chNo:	// not working
		if (cchTime + 500 > now && (rUpdate != 0)) {

			chState = chYes;
			xmprintf(3, "RcvInfo::rchUpdate()   receiver channel %d started  [ %d -> %d]  (now = %u; cchTime = %u)\r\n", chID, s, chState, now, cchTime);
		}
		break;
	};

	prevCchTime = cchTime;
}

RcvInfo ri[rcc];
#define logReceiverInfo

static char chSym[] = {'#', '.', ',', '@', '^'};

void ReceiverChannel::rcvUpdate() {
	//if (!chID) return; //   should be not 0
	if( digitalReadFast(pin) == HIGH ) { 
		if (resetCounter == 0) {						// started already
			period = chPeriod;							//  time from previous HIGH
			if (period < rpMin || period > rpMax) {  	//  outlier
				outlierFlag1 = 1;						
				if (outlierCounter == 0) {				//   first outlier
					outlierCounter = 10;
				} else {
					outlierCounter -= 1;
					if (outlierCounter == 0) {			//  out of sync..    what happened?
						chPeriod = 0;					//  setup 'new beginnig'
					}
				}

				//xmprintf(17, "%c", chSym[chID]);
				xmprintf(17, "(%d)  period=%d; rpMin=%d; rpMax = %d \r\n", chID, period, rpMin, rpMax);
			} else {									//  looks good
				outlierFlag1 = 0;
				outlierCounter = 0;
				wch = chTime;
				chTime = 0;
				chPeriod = 0;
			} 	
		} else {										//   not started yet
			period = chPeriod;
			wch = chTime;
			chPeriod = 0;
			chTime = 0;

			rp = period;
			rpMin = rp * 0.85f;
			rpMax = rp * 1.15f;

		}
	}  else { 											//  LOW
		cchCopy = chTime;
		if (resetCounter == 0) {						//  started already
			if (outlierFlag1 == 0) {
				if (cchCopy < rvMin || cchCopy > rvMax) {    	//  outlier
					outlierFlag2 = 1;
					xmprintf(17, "(%d)  cchCopy=%d; rvMin=%d; rvMax = %d \r\n", chID, cchCopy, rvMin, rvMax);
				} else {
					cch = cchCopy;
					cchTime = millis();
					rUpdate = 1;
					outlierFlag2 = 0;
				}

#ifdef logReceiverInfo
				xqm::XQMData4 cInfo;
				cInfo.id = chID;
				cInfo.timestamp = cchTime;
				cInfo.data[0] = wch;
				cInfo.data[1] = cch;
				cInfo.data[2] = outlierFlag1;
				cInfo.data[3] = outlierFlag2;

				lfSendMessage<xqm::XQMData4>(&cInfo);
#endif
			}
		} else {
			rv = cchCopy;
			//rvMin = rv * 0.5f;
			//rvMax = rv * 1.5f;
			switch (chID) {
			case 1: rvMin = steerMinMks - 200; rvMax = steerMaxMks + 200; break;
			case 2: rvMin = motorMinMks - 200; rvMax = motorMaxMks + 200; break;
			};

			resetCounter -= 1;
			if (resetCounter == 0) {					//  starting
				xmprintf(3, "rcv ch %d starting; rv = %d; rvMin = %d; rvMax = %d;   rp = %d; rpMin = %d; rpMax = %d \r\n", 
					chID, rv, rvMin, rvMax, rp, rpMin, rpMax);
			}
		}
	}
}

void RcvInfo::rcvPrint() {
	xmprintf(3, "receiver CH %d cch = %d   wch = %d \r\n", chID, cch, wch);
}

void RcvInfo::setupRange(int minMks, int maxMks, int zeroMks) {
	int i = chID - 1; //  0 and 1
	NVIC_DISABLE_IRQ(IRQ_GPIO6789);
	ch[i].rangeZeroMks = zeroMks;
	int d1 = zeroMks - minMks;
	int d2 = maxMks - zeroMks;
	int d = (d1 < d2) ? d2 : d1;

	ch[i].rangeMinMks = zeroMks - d;
	ch[i].rangeMaxMks = zeroMks + d;
	NVIC_ENABLE_IRQ(IRQ_GPIO6789);
}

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
		ri[i].cchTime = ch[i].cchTime;

		ri[i].rUpdate = ch[i].rUpdate;
		ch[i].rUpdate = 0;
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

void RcvInfo::riSetup(int id) {
	chID = id;
	chState = chUnknown;
	int i;
	for (i = 0; i < vhSize; i++) {
		vh[i] = 90.0; 
	}
}

void receiverSetup() {
	xmprintf(1, "receiverSetup() .... ");
	
	ch[0].pin = rcv_ch1pin;  ch[0].chID = 1;   	
	ch[1].pin = rcv_ch2pin;	 ch[1].chID = 2;	

	ri[0].riSetup(1);
	ri[1].riSetup(2);

	ri[0].deadBand = 5;
	ri[1].deadBand = 8;

	ri[0].setupRange(steerMinMks, steerMaxMks, steerZeroMks);
	ri[1].setupRange(motorMinMks, motorMaxMks, motorZeroMks);

	pinMode(rcv_ch1pin, INPUT_PULLDOWN); 
	pinMode(rcv_ch2pin, INPUT_PULLDOWN); 

	//pinMode(rcv_ch1pin, INPUT_PULLUP); 
	//pinMode(rcv_ch2pin, INPUT_PULLUP); 

	//pinMode(rcv_ch1pin, INPUT); 
	//pinMode(rcv_ch2pin, INPUT); 

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
	int q = chID - 1;

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
					setupRange(ch[q].rangeMinMks, ch[q].rangeMaxMks, c.rccInfo.center);
					chState = chYes;
					ledstripMode(lsManualControl, 128, 0x00007500);
					xmprintf(3, "zero calibration ch %d completed [%d - %d - %d]  (chState = chYes)\r\n", chID, ch[q].rangeMinMks, ch[q].rangeZeroMks, ch[q].rangeMaxMks);
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



