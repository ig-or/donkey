



#include "receiver.h"
#include "elapsedMillis.h"
#include "ttpins.h"
#include "teetools.h"

//the number of microseconds between the pulses
volatile unsigned long wch1 = 0;
volatile unsigned long wch2 = 0;

//the number of microseconds of the pulse
volatile unsigned long cch1 = 0;
volatile unsigned long cch2 = 0;

elapsedMicros ch1Time;
elapsedMicros ch2Time;

void checkCH1(){
   if( digitalReadFast(rcv_ch1pin) == HIGH ) { 
	wch1 = ch1Time;
	ch1Time = 0;
   }   else { 
	cch1 = ch1Time;
   }
}
void checkCH2(){
   if( digitalReadFast(rcv_ch2pin) == HIGH ) { 
	wch2 = ch2Time;
	ch2Time = 0;
   }   else { 
	cch2 = ch2Time;
   }
}

void receiverPrint() {
	xmprintf(3, "receiver cch1 = %lu, wch1 = %lu; \t  cch2 = %lu, wch2 = %lu  \r\n", cch1, wch1, cch2, wch2);
}

void receiverSetup() {
	attachInterrupt( rcv_ch1pin, checkCH1, CHANGE );  
	attachInterrupt( rcv_ch2pin, checkCH2, CHANGE );  
}


