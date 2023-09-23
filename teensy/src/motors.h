

#pragma once

const int motorZeroRate = 90;
const int motorStopDB = 19;
const int motorRange = motorZeroRate - motorStopDB;

/**
 * forward backward or stop.
*/
enum MotorControlState {
	mcUnknown,
	mcStop,
	mcForward,
	mcBackward,
	mcStatesCount
};
// MotorControlState getCurrentMcState(int a);

typedef void (*onMotorStateChangedP)(MotorControlState anotherMotorState);
void setupMotorStateChangedCallback(onMotorStateChangedP f);
MotorControlState getCurrentMotorState();
void msetup();

/***
 *  steering.
*/
void steering(int angle);
void moveTheVehicle(int a);


/**shift the gear
 * \param gear 1 furst, 2 second
*/
void mshift(int gear);