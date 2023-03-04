

#include "PWMServo.h"
#include "motors.h"
#include "ttpins.h"

PWMServo steeringServo;
PWMServo motor;
PWMServo tshift;

const int steerMaxMks = 1780;
const int steerMinMks = 1294;

const int motorMaxMks = 1780;
const int motorMinMks = 1294;

const int shiftMaxMks = 1780;
const int shiftMinMks = 1294;

void msetup() {
	steeringServo.attach(wheel_servo_pin, steerMinMks, steerMaxMks);
	motor.attach(motor_control_pin);
	tshift.attach(transmission_shift_pin);
}

void steering(int angle) {
	steeringServo.write(angle);
}




