



#include "receiver.h"
#include "elapsedMillis.h"
#include "ttpins.h"
#include "teetools.h"

const int receiverChannelCount = 2;
int delta = 2;

struct ReceiverChannel {
	int chID = 0; 
	volatile long wch = 0; //the number of microseconds between the pulses
	volatile  long cch = 0; //the number of microseconds of the pulse
	volatile  long cchPrev = 0; //the number of microseconds of the pulse
	volatile bool working = false;
	elapsedMicros chTime;
	recvChangeT cb = 0;		//  callback function
	int pin = 0;
	void rcvUpdate();
	void rcvPrint();
};

void ReceiverChannel::rcvUpdate() {
	if (!chID) return; //   should be not 0
   if( digitalReadFast(pin) == HIGH ) { 
	wch = chTime;
	chTime = 0;
   }   else { 
	cch = chTime;
		if (cb != 0) {
			if ((cch > (cchPrev + delta)) || (cch < (cchPrev - delta))) {
				cchPrev = cch;
				if (cch != 0) {
					if (working) {
						cb(receiverFlagOK, cch);
					} else {
						working = true;
						cb(receiverFlagControlStarted, cch);
					}
				}
			}
		}
	}
}

void ReceiverChannel::rcvPrint() {
	xmprintf(3, "receiver CH %d cch = %d   wch = %d \r\n", chID, cch, wch);
}

ReceiverChannel ch[receiverChannelCount];

void checkCH1() {
	ch[0].rcvUpdate();
}
void checkCH2() {
	ch[1].rcvUpdate();
}

/*
unsigned long rcv_ch1() {
	return cch1;
}
unsigned long rcv_ch2() {
	return cch2;
}
*/


void receiverPrint() {
	//return;
	ch[0].rcvPrint();
	ch[1].rcvPrint();
}

void receiverSetup() {
	//;
	ch[0].pin = rcv_ch1pin;  ch[0].chID = 1;
	ch[1].pin = rcv_ch2pin;	 ch[1].chID = 2;

	attachInterrupt( rcv_ch1pin, checkCH1, CHANGE );  
	attachInterrupt( rcv_ch2pin, checkCH2, CHANGE );  
}

void setReceiverUpdateCallback(recvChangeT f, int chNum) {
	switch (chNum) {
	case 1: ch[0].cb = f; break;
	case 2: ch[1].cb = f; break;
	default: xmprintf(3, "setReceiverUpdateCallback error \n");
	};
}
