

#include "PWMServo.h"
#include "motors.h"
#include "ttpins.h"
#include "sr04.h"
#include "teetools.h"

#include "wiring.h"

PWMServo steeringServo;
PWMServo motor;
PWMServo tshift;

static const int steerMaxMks = 1780;
static const int steerMinStopMks = 1540;
static const int steerMaxStopMks = 1550;
static const int steerMinMks = 1294;

static const int motorMaxMks = 1740;
static const int motorMaxStopMks = 1560;
static const int motorMinStopMks = 1540;
static const int motorMinMks = 1259;

static const int shiftMaxMks = 1780;
static const int shiftMinMks = 1294;



void msetup() {
	steeringServo.attach(wheel_servo_pin, steerMinMks, steerMaxMks);
	motor.attach(motor_control_pin, motorMinMks, motorMaxMks);
	tshift.attach(transmission_shift_pin, shiftMinMks, shiftMaxMks);
}

void steering(int angle) {
	steeringServo.write(angle);
}


/**
 * \param a the speed, from 0 to 180.  90 is stop.
*/
void moveTheVehicle(int a) {

	motor.write(a);
}




