
#pragma once

/**
 *  some useful iinfo about receiver states.
*/
enum ReceiverUpdateFlag {
	receiverFlagOK,					//  normal operation
	receiverFlagControlStarted,		//  the transmitter started
	receiverFlasControlStopped		//  the transmitter stopped
};

/**
 * callback function.
 * first parameter is the ReceiverUpdateFlag
 * second is the channel value
*/
typedef void (*recvChangeT)(ReceiverUpdateFlag, int);

void receiverSetup();
void receiverPrint();
//unsigned long rcv_ch1();
//unsigned long rcv_ch2();

/**
 * setup a function which will be called when receiver value will change
 * \param the callback function
 * \param channel number (1 or 2)
*/
void setReceiverUpdateCallback(recvChangeT f, int chNum = 1);