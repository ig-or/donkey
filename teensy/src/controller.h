

#pragma once

void control100();
void controlSetup();
void controlPrint();
void cPrintRcvInfo(bool print);

/**
 *  \param u  0 - disable the motor.
*/
void enableMotor(int u);
void mSetSpeed(int s);