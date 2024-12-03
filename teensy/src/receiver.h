/// (control radio) receiver interface

#pragma once

#include "xmfilter.h"

/// @brief how to calibrate the receiver channels
enum ChCalibrationMethod {
	rcNoCalibration,
	rcCalibrationOnlyZero,
	rcFullCalibration
};

/**
 * state of the calibration status for receiver channel.
*/
enum CalibrationState {
	rcNo,					//  not calibrated
	rcCalibrationStarting,
	rcGettingMiddle,
	rcMovingDown,
	rcMovingDown2,
	rcGettingDown, 
	rcMovingUp,
	rcMovingUp2,
	rcGettingUp,
	rcComplete				//  calibrated
};

enum RcvChannelState {
	chUnknown,
	chCalibration,
	chYes,  //  working
	chNo	// not working
};

struct RCCalibrationInfo {
	int center = 0;
	int minimum = 0;
	int maximum = 0;
	void rciReset() { center = 0; minimum = 0; maximum = 0; }
};

struct RCHCalibration {
	ChCalibrationMethod cMethod = rcCalibrationOnlyZero;
	unsigned int calibrationStartTimeMs = 0;
	unsigned int calibrationStateTimeMs = 0;
	unsigned int calibrationDataTimeMs = 0;
	int calibrationDataCounter = 0;
	CalibrationState cState = rcNo;
	RCCalibrationInfo rccInfo;
	XCov1 xcov;
	void startReceiverChannelCalibration(unsigned int now);
	void calibrationFailed();
	void zeroCalibrationComplete();
};

struct RcvInfo {
	long wch = 0;						/// copy of the ReceiverChannel
	long cch = 0;						/// copy of the ReceiverChannel
	unsigned int cchTime = 0;			/// copy of the ReceiverChannel

	int chID = 0;						/// > 0
	int v = 0;							/// value scaled from 0 to 180
	RcvChannelState chState = chUnknown;

	unsigned int prevCchTime = 0;
	RCHCalibration calibration;
	int deadBand = 0;
	int rangeMinMks = 0;
	int rangeZeroMks = 0;
	int rangeMaxMks = 0;

	/// @brief update everything based on wch, cch and cchTime info.
	void rchUpdate(unsigned int now);
		/**
	 * setup receiver changing range 
	*/
	void setupRange(int minMks, int maxMks, int zeroMks);
	/// @brief   print out receiver statistics
	void rcvPrint();					
	void processCalibration(unsigned int now);
};

/**
 *  some useful iinfo about receiver states.
*/

const int rcc = 2; ///< receiver channel count

extern RcvInfo ri[rcc];

void receiverSetup();
void receiverPrint();
void rcvProcess(unsigned int now);   //    call it from 255 level interrupt

void receiverProcessCalibration(unsigned int now);

