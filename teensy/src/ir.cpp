/**
 *     IR receiver
 */

#include <Arduino.h>

#define SUPPRESS_ERROR_MESSAGE_FOR_BEGIN
#include <IRremote.h>

//#include "cmdhandler.h"
#include "teetools.h"
#include "ttpins.h"
#include "ir.h"


static  IRrecv irrecv(IR_RECV_PIN);
//static decode_results irResults;
const int ciSize = 10;	///< how many buttons we know
const int cInterval_ms = 250; ///< minimum time between the commans
uint32_t ciTime[ciSize]; ///< command times in milliseconds

///  the commands
const char* cmds[ciSize] = {"dec speed", 	"play", 	"inc speed", 	"setup", 	"up", 
							"stop", 		"left", 	"enter", 		"right", 	"down"};
///   command codes
enum IRCodes {
	VOLm 	= 0xfd00ff,
	PLAY 	= 0xfd807f,
	VOLp 	= 0xfd40bf,

	SETUP 	= 0xfd20df,
	UP 		= 0xfda05f,
	STOP 	= 0xfd609f,

	LEFT 	= 0xfd10ef,
	ENTER 	= 0xfd906f,
	RIGHT 	= 0xfd50af,
	DOWN 	= 0xfdb04f
};
//unsigned long ci[ciSize] = {VOLm, PLAY, VOLp, SETUP, UP, STOP, LEFT, ENTER, RIGHT, DOWN};

void irSetup() {
	int i;
	for (i = 0; i < ciSize; i++) {
		ciTime[i] = 0;
	}
	irrecv.enableIRIn(); // Start the receiver
}

void irProces() {
	//if (irrecv.decode(&irResults)) {
	if (irrecv.decode()) {
		int cIndex = -1;
    	//Serial.println(results.value, HEX);
		//unsigned long code = irResults.value;
		
    	irrecv.resume(); // Receive the next value
		unsigned long code = IrReceiver.decodedIRData.command;
		xmprintf(0, "IR: cmd: %x;  addr: %x;  numberOfBits=%d; flags = %u; protocol = %d\r\n", 
			code, IrReceiver.decodedIRData.address, IrReceiver.decodedIRData.numberOfBits,
			IrReceiver.decodedIRData.flags, IrReceiver.decodedIRData.protocol);

		switch (code) {
			case VOLm:  cIndex = 0; break;
			case PLAY:  cIndex = 1; break;
			case VOLp: 	cIndex = 2; break;
			case SETUP: cIndex = 3; break;
			case STOP: 	cIndex = 5; break;
			case LEFT: 	cIndex = 6; break;
			case ENTER: cIndex = 7; break;
			case RIGHT:  cIndex = 8; break;
			default: break;
		}
		if (cIndex < 0) { // wrong code
			return;
		}
		uint32_t t1 = ciTime[cIndex]; //  previous time for this code
		if (msNow == t1) {   // time is the same, so lets not repeat it
			return;
		}
		uint32_t dt;
		if (msNow > t1) { //  normal (no rollover)
			dt = msNow - t1;

		} else {  //   49 rollover?
			dt = msNow; // should be OK
		}
		if (dt > cInterval_ms) { // previous commamd was some time ago
			ciTime[cIndex] = msNow;  //  remember cmd time
			//processTheCommand(cmds[cIndex]); // process the command
		}
  	}
}

