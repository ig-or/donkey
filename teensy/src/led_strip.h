/**
 * @file led_strip.h
 * @author your name (you@domain.com)
 * @brief  led stripe control
 * @version 0.1
 * @date 2024-03-01
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

/**
 * @brief different modes of the operation 'in general'
 * 
 */
enum LSFlashMode {
	lsOff,
	lsRun3,
	lsMovingForward,		///   moving forward right now
	lsMovingBackward,
	lsFrontObstacle,		/// front obstacle detected
	lsRearObstakle,
	lsStop,					///   we are not moving
	lsModesCount
};
/// @brief  setup everything
void ledstripSetup();
void ledstripTest(unsigned int now);
/// @brief  call this from 'main' periodically (at low priority)
/// @param now current time in milliseconds
void ledstripProcess(unsigned int now);

/**
 * activates led strip mode.
 * \param mode the mode
 * \param intensity 0 means deactivate.. 255 is the maximum
*/
void ledstripMode(LSFlashMode mode, unsigned char intensity);
