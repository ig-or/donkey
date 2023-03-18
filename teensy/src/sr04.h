

#pragma once

struct SR04Info  {
	unsigned int measurementTimeMs = 0; //  the timestamp
	unsigned int distance_mm = 0; //  measured distance in millimeters
	char quality = 0;		//  0 -> quality is bad
};

/**
 * setup all the SR04 things.
*/
void usSetup();

/**
 * \param sid sensor ID, 0 (forward) or 1 (backward)
*/
void usStartPing(char sid);
void usPrint();

/**
 * get the measurements from the sensor.
 * \param[out] info where to put the info
 * \param[in] sid  sensor ID  ;  0 or 1
*/
void getSR04Info(char sid, SR04Info& info);

/**
 * get the measurements from the sensor, with median filtering enabled.
 * \param[out] info where to put the info
 * \param[in] sid  sensor ID  ;  0 or 1
*/
//void getSR04Info2(char sid, SR04Info& info);
