
#pragma once

/**
 *  some useful iinfo about receiver states.
*/

const int rcc = 2; ///< receiver channel count
/*
enum ReceiverUpdateFlag {
	receiverFlagOK,					//  normal operation
	receiverFlagControlStarted,		//  the transmitter started
	receiverFlasControlStopped		//  the transmitter stopped
};
*/
/**
 * callback function.
 * first parameter is the ReceiverUpdateFlag
 * second is the channel value
*/
//typedef void (*recvChangeT)(ReceiverUpdateFlag, int);

/**
 * a receiver callback function.
 * \param v the value from 0 (minMks) to 180 (maxMks)
*/
typedef void (*recvChangeT)(int);

void receiverSetup();
void receiverPrint();

void startReceiverCalibrate();

void receiverProcess(unsigned int now);
//unsigned long rcv_ch1();
//unsigned long rcv_ch2();

/**
 * setup a function which will be called when receiver value will change
 * \param f the callback function
 * \param chNum channel number (1 or 2)
*/
void setReceiverUpdateCallback(recvChangeT f, int chNum = 1);