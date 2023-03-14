

#include "PWMServo.h"
#include "motors.h"
#include "ttpins.h"
#include "sr04.h"

PWMServo steeringServo;
PWMServo motor;
PWMServo tshift;

const int steerMaxMks = 1780;
const int steerMinMks = 1294;

const int motorMaxMks = 1740;
const int motorMinMks = 1259;

const int shiftMaxMks = 1780;
const int shiftMinMks = 1294;

void msetup() {
	steeringServo.attach(wheel_servo_pin, steerMinMks, steerMaxMks);
	motor.attach(motor_control_pin, motorMinMks, motorMaxMks);
	tshift.attach(transmission_shift_pin, shiftMinMks, shiftMaxMks);
}

void steering(int angle) {
	steeringServo.write(angle);
}

int wallDetector(int a) {
	
}

void moveTheVehicle(int a) {
	motor.write(a);
}




