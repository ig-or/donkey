

#pragma once

/** \struct SR04Info
 * measurement result of the us sensor.
*/
struct SR04Info  {
	unsigned int measurementTimeMs = 0; //  the timestamp
	unsigned int distance_mm = 0; 		//  measured distance in millimeters
	char quality = 0;					///<  0 -> quality is bad; 1 normal measurement; 2-> too far; 3-> bad data
};

/**
 * setup all the SR04 things.
*/
void usSetup();

/**
 * \param sid sensor ID, 0 or 1
*/
void usStartPing(char sid);
void usPrint();

/**
 * get the measurements from the ultrasound sensor.
 * \param[out] info where to put the info
 * \param[in] sid  sensor ID  ;  0 or 1
*/
void getSR04Info(char sid, SR04Info& info);
/**
 * get the measurements from the ultrasound sensor.
 * do some median filtering.
 * \param[out] info where to put the info
 * \param[in] sid  sensor ID  ;  0 or 1
*/
void getSR04Info2(char sid, SR04Info& info);
