
#include <Arduino.h>
//#include "ADC.h"

#include "ttpins.h"
#include "ttsetup.h"
#include "teetools.h"
#include "led_indication.h"

#include "cmdhandler.h"
#include "adcsetup.h"
#include "memsic.h"
#include "receiver.h"
#include "motors.h"
#include "ir.h"
#include "sr04.h"
//#include "logfile.h"
#include "eth.h"
#include "power.h"

//#include "xmfilter.h"
//#include "imu_alg.h"

//#include "EventResponder.h"

void event100ms() {  //   call this 10 Hz
	//analogWrite(led1_pin, led1.liGet(msNow)); 
	analogWrite(ledPin, led1.liGet(msNow));  //   support the orange LED
	//irProces();

	//unsigned long ch1 = rcv_ch1();
	//unsigned long val = map(ch1, 1290, 1780, 0, 180);
	//steering(val);
	usPing();
}

static int lastBatteryInfo = 0, nextBatteryInfo = 0;

void event250ms() {	  //  call this 4  Hz
	h_usb_serial();	

	if (lastBatteryInfo != nextBatteryInfo) {
		lastBatteryInfo = nextBatteryInfo;
		batteryUpdate(lastBatteryInfo);
	}
	
}

void event1s() {   //  call this 1 Hz
	usPrint();
	//xmprintf(1, "1s EVENT \r\n");
	//xmprintf(0, ".");
	//batteryUpdate();
	//adc->readSingle(1);
	//bool test1 = adc->adc1->startSingleRead(m1cs);

	bool test2 = adcStartSingleRead(0, bvPin);  //adc->adc0->startSingleRead(bvPin);


	//xmprintf(0, " .%d,%d ", test1?1:2, test2?1:2);

	//float p = getBattPercent();
	//if (p < 10.0f) {
	//	led1.liSetMode(LedIndication::LIRamp, 8.0);
//
	//}
	//receiverPrint();
}

void powerStatusChangeH(PowerStatus from, PowerStatus to) {
	xmprintf(0, "POWER: %s -> %s \r\n", pwrStatusText[from], pwrStatusText[to]);
	mxat(from != to);
	switch(to) {
		case pwVeryLow:
			//mdStop();
			//canUseMotors(false);
			led1.liSetMode(LedIndication::LIRamp, 8.0);
			break;

		default:
			if (from == pwVeryLow) {
				//canUseMotors(true);
				led1.liSetMode(LedIndication::LIRamp, 0.8);
			}
	};
}

void onAdc0() {
	nextBatteryInfo =  adcReadSingle(0);
}
void rcv_ch1(ReceiverUpdateFlag flag, int v) {
	unsigned long val = map(v, 1294, 1780, 0, 180);
	steering(val);
	xmprintf(3, "CH1\t v = %d; val = %d  \r\n", v, val);
}

void rcv_ch2(ReceiverUpdateFlag flag, int v) {
	unsigned long val = map(v, 1259, 1740, 0, 180);
	moveTheVehicle(val);
	xmprintf(3, "CH2\t v = %d; val = %d  \r\n", v, val);
}



extern "C" int main(void) {

	xmprintf(1, "\r\n4.1 started\r\n");
	
	pinMode(13, OUTPUT);
	/*
	while (1) {
		digitalWriteFast(13, HIGH);
		delay(500);
		digitalWriteFast(13, LOW);
		delay(500);

		xmprintf(1, "HELLO! \n");
	}
*/
	//xmprintf(1, "starting ttsetup \n");
	ttSetup();
	xmprintf(1, "ttsetup OK \n");
	msNow = millis();
	uint32_t fast100msPingTime = msNow;
	uint32_t fast250msPingTime = msNow;
	uint32_t oneSecondPingTime = msNow;
	uint32_t secondsCounter = 0;

	serPwStatusChangeHandler(powerStatusChangeH);
	setAdcHandler(onAdc0, 0);
	led1.liSetMode(LedIndication::LIRamp, 1.2);

	setReceiverUpdateCallback(rcv_ch1, 1);
	setReceiverUpdateCallback(rcv_ch2, 2);

	xmprintf(1, "entering WHILE \n");
	while (1) {
		msNow = millis();
		//mksNow = micros();

		//analogWrite(ledPin, p);
		//analogWrite(led1_pin, led1.liGet(msNow));

		//delay(2);
		//axSmoothCounter = 0;
		//processMemsicData(imuInfo);
		//if (axSmoothCounter != 0) {
		//	setMSpeed(axSmoothed, axSmoothed);
		//}

		ethLoop();		//  ethernet
		//lfProcess();  	//  log file

		if (msNow > (fast100msPingTime + 100)) {
			fast100msPingTime = msNow;
			event100ms();
			if (msNow > (fast250msPingTime + 250)) {
				fast250msPingTime = msNow;
				event250ms();
				if (msNow > (oneSecondPingTime + 1000)) {
					oneSecondPingTime = msNow;
					++secondsCounter;
					event1s();
				}
			}
		}
		yield();	
	}
}


