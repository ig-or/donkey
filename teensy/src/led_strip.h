
#pragma once

enum LSFlashMode {
	lsOff,
	lsRun3,
	lsMovingForward,
	lsMovingBackward,
	lsFrontObstacle,
	lsRearObstakle,
	lsStop,
	lsModesCount
};

void ledstripSetup();
void ledstripTest(unsigned int now);
void ledstripProcess(unsigned int now);

/**
 * activates led strip mode.
 * \param mode the mode
 * \param intensity 0 means deactivate.. 
*/
void ledstripMode(LSFlashMode mode, unsigned char intensity);
