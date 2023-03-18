

#pragma once


void msetup();

void steering(int angle);
void moveTheVehicle(int a);
void enableMotor(int u);

/**shift the gear
 * \param gear 1 furst, 2 second
*/
void mshift(int gear);