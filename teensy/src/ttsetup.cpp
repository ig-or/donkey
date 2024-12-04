#include <Arduino.h>

#include "ttpins.h"
#include "ttsetup.h"
#include "adcsetup.h"
//#include "motordriver.h"
#include "ir.h"
#include "teetools.h"
#include "memsic.h"
#include "eth.h"
#include "logfile.h"
#include "receiver.h"
#include "motors.h"
#include "sr04.h"
#include "controller.h"
//#include "imu_alg.h"
#include "led_strip.h"

int ttSetup() {
	pinMode(imuPwrPin, OUTPUT);
	pinMode(ledPin, OUTPUT);
	pinMode(led1_pin, OUTPUT);

	digitalWriteFast(imuPwrPin, LOW); //    no power for IMU yet

	//   from https://www.pjrc.com/teensy/td_pulse.html, see the bottom of the page
	analogWriteResolution(pwmResolution);

	setupADC();
	//////analogReadAveraging(8);
	
	lfInit();  //  SD log file init; will set up sdStarted = true; if SD card present
	

	//irSetup(); // IR receiver setup

	
	analogWrite(led1_pin, maxPWM);
	ledstripSetup();
	ethSetup();


	//setupMemsic();
	//imuAlgInit();
	receiverSetup();

	msetup();
	usSetup();
	controlSetup();
	analogWrite(led1_pin, 0);

	// switch/button
	pinMode(switchPin, INPUT);

	NVIC_SET_PRIORITY(IRQ_GPIO6789, 54);  //  setup high priiority on all IO pins; should not be below 32!!
	
	return 0;
}

