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

int ttSetup() {
	pinMode(imuPwrPin, OUTPUT);
	digitalWriteFast(imuPwrPin, LOW); //    no power for IMU yet

	pinMode(ledPin, OUTPUT);

	//   from https://www.pjrc.com/teensy/td_pulse.html, see the bottom of the page
	analogWriteResolution(pwmResolution);

	setupADC();
	//////analogReadAveraging(8);
	
	lfInit();  //  SD log file init; will set up sdStarted = true; if SD card present
	ethSetup();

	//irSetup(); // IR receiver setup

	pinMode(led1_pin, OUTPUT);
	analogWrite(led1_pin, maxPWM);
	//setupMemsic();
	//imuAlgInit();
	receiverSetup();

	msetup();
	usSetup();
	controlSetup();
	analogWrite(led1_pin, 0);
	
	return 0;
}

