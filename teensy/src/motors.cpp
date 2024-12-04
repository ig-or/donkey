

#include "PWMServo.h"
#include "motors.h"
#include "ttpins.h"
#include "sr04.h"
#include "teetools.h"
#include "xmfilter.h"
#include "led_strip.h"

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

static const int motorMaxMks = 1750;
//static const int motorMaxStopMks = 1560;
//static const int motorMinStopMks = 1540;
static const int motorMinMks = 1350;

static const int shiftMaxMks = 2200;
static const int shiftMinMks = 800;

static const int shift_1 = 172; //  first gear value
static const int shift_2 = 18;	// second gear value

static PolyFilter<3> mpf_25_100;	// low pass filter for motor output
static PolyFilter<3> mpf_08_50;	// low pass filter for motor output
static PolyFilter<3> mpf_027_50;

static const int db = 1;
static int prevMotorValue = 0;
MotorControlState prevMotorState = mcUnknown;
onMotorStateChangedP onMotorStateChanged = 0;

const char* motorStateText[mcStatesCount] = {"unknown", "STOP", "FORWARD", "BACKWARD"};

MotorControlState getCurrentMotorState() {
	return prevMotorState;
}

/**
 * \param a new code
*/
void processMotorState(int a) {
	MotorControlState s = mcUnknown;
	const int db2 = 4;
	switch (prevMotorState) {
		case mcUnknown:
		case mcStop:
			if (a < motorZeroRate - motorStopDB - db2) {
				s =  mcForward;
			} else if (a > motorZeroRate + motorStopDB + db2) {
				s = mcBackward;
			} else {
				s = mcStop;
			}			
			break;
		case mcForward: 
			if (a > motorZeroRate + motorStopDB + db2) {
				s = mcBackward;
			} else if (a > motorZeroRate - motorStopDB + db2) {
				s = mcStop;
			} else {
				s =  mcForward;
			}
			break;
		case mcBackward: 
			if (a < motorZeroRate - motorStopDB - db2) {
				s = mcForward;
			} else if (a < motorZeroRate + motorStopDB - db2) {
				s = mcStop;
			} else {
				s =  mcBackward;
			}
		break;
	}

	if (s != prevMotorState) {
		xmprintf(3, "motor %s -> %s \r\n", motorStateText[prevMotorState], motorStateText[s]);
		if (onMotorStateChanged != 0) {
			onMotorStateChanged(s);
		}
		prevMotorState = s;
	}
}

/**
 * \param a the speed, from 0 to 180.  90 is stop.
*/
/*
MotorControlState getCurrentMcState(int a) {
	MotorControlState state;

	if (a < motorZeroRate - motorStopDB) {
		state =  mcForward;
	} else if (a > motorZeroRate + motorStopDB) {
		state = mcBackward;
	} else {
		state = mcStop;
	}
	return state;
}
*/

void setupMotorStateChangedCallback(onMotorStateChangedP f) {
	onMotorStateChanged = f;
}


void msetup() {
	xmprintf(1, "msetup ... .. ");
	
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
	/*switch (mMode) {
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
	*/
	steeringServo.write(angle);
	steeringCounter += 1;
}

/**
 * \param a the speed, from 0 to 180.  90 is stop.
*/
void moveTheVehicle(int a) {
	if (a > 180) {
		a = 180;
	} else if (a < 0) {
		a = 0;
	}
	if (abs(a - prevMotorValue) > db) {
		prevMotorValue = a;
		processMotorState(a);
		motor.write(a);
		//xmprintf(3, " a=%d \r\n", a);
	}
}




