

#include "PWMServo.h"
#include "motors.h"
#include "ttpins.h"
#include "sr04.h"
#include "teetools.h"
#include "xmfilter.h"

#include "wiring.h"
#include <cmath>
#include <cstdlib>

static PWMServo steeringServo;
static PWMServo motor;
static PWMServo tshift;

static const int steerMaxMks = 1780;
static const int steerMinStopMks = 1540;
static const int steerMaxStopMks = 1550;
static const int steerMinMks = 1294;

static const int motorMaxMks = 1740;
static const int motorMaxStopMks = 1560;
static const int motorMinStopMks = 1540;
static const int motorMinMks = 1259;

static const int shiftMaxMks = 2200;
static const int shiftMinMks = 800;

static const int shift_1 = 172; //  first gear value
static const int shift_2 = 18;	// second gear value

static PolyFilter<3> mpf_25_100;	// low pass filter for motor output
static PolyFilter<3> mpf_08_50;	// low pass filter for motor output
static PolyFilter<3> mpf_027_50;

static const int db = 1;
static int prevMotorValue = 90;

enum MotorMode {
	mmStop = 0, 
	mmRunning_027,  // low pass filter 0.27 Hz
	mmRunning_05, 	 // low pass filter 0.5 Hz
	mmRunning_08, 	 // low pass filter 0.8 Hz
	mmRunning_25,   // low pass filter 2.5 Hz

	mmShiftTest
};

static MotorMode mMode = mmStop;
PolyFilter<3> mpf;

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

void msetup() {
	xmprintf(1, "msetup ... .. ");
	enableMotor(mmStop);
	mpf.pfInit(ca3_08_50, cb3_08_50);

	steeringServo.attach(wheel_servo_pin, steerMinMks, steerMaxMks);
	motor.attach(motor_control_pin, motorMinMks, motorMaxMks);
	tshift.attach(transmission_shift_pin, shiftMinMks, shiftMaxMks);
	delay(10);
	tshift.write(shift_1);

	xmprintf(17, "... OK \r\n");
}

void mshift(int gear) {
	switch (gear) {
	case 1: tshift.write(shift_1);  break;
	case 2: tshift.write(shift_2);  break;
	default: ;
	};
}

static unsigned int steeringCounter = 0;
void steering(int angle) {
	switch (mMode) {
		case mmStop:  break;
		case mmShiftTest: {
				int aa = (angle - 90) * 2 + 90;
				if (aa < 0) aa = 0;
				if (aa > 180) aa = 180;
				tshift.write(aa); 
				if (steeringCounter % 20 == 0) {
					xmprintf(3, "mmShiftTest \t%d\r\n", aa);
				}
			}
		break;

		default: 	steeringServo.write(angle);
	};
	steeringCounter += 1;
}


/**
 * \param a the speed, from 0 to 180.  90 is stop.
*/
void moveTheVehicle(int a) {
	if (mMode == mmStop) {
	} else {
		long aa = std::lround(mpf.pfNext(a));
		if (abs(aa - prevMotorValue) > db) {
			prevMotorValue = aa;
			motor.write(aa);
		}
	}
}




