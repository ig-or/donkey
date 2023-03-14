

#include "sr04.h"
#include "ttpins.h"
#include "teetools.h"

#include "core_pins.h"
#include "avr/pgmspace.h" 
#include "IntervalTimer.h"
#include "TeensyTimerTool.h"

struct SR04 {
	int id = 0;				//  id of this device
	int trigPin;			//   trigger pin
	int echoPin;			//  echo pin
	int max_cm_distance;	// max distance in cm
	int maxEchoTimeMks = 0;	//   maximum echo time in mks
	//int minEchoTimeMks = 0;	//   minimum echo time in mks
	SR04Info info; //  measurement result
	unsigned int echoStartTimeMks = 0;
	unsigned int echoStopTimeMks = 0;
		
	/**
	 * \param id its ID
	 * \param tp trig pin number
	 * \param ep echo pin
	 * \param md maximum diistance in cm
	*/
	void srSerup(int id_, int tp, int ep, int md = 400);
};
const int nsr = 2;
SR04 sr04[nsr];

#define US_ROUNDTRIP_CM 57      // Microseconds (uS) it takes sound to travel round-trip 1cm (2cm total), uses integer to save compiled code space. Default=57
#define trigPulsTimeMks 16		// the length of the trigger pulse

//IntervalTimer pingTimer;//  this is for trigger pulse generation (kind of overkill)
TeensyTimerTool::OneShotTimer trigTimer(TeensyTimerTool::TMR4);
TeensyTimerTool::PeriodicTimer periodicPingTimer(TeensyTimerTool::TMR4);
volatile char pingTimerStatus = 4;
/*volatile */char currentID = -1;
volatile unsigned int pingErrorCounter = 0;

unsigned int errorsCounter[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void usPrint() {
	xmprintf(3, "SR04\tquality: (%d, %d)  distance: [%d  %d] mm,  errors=[%u  %u  %u  %u  %u  %u  %u  %u  %u  %u]\r\n",
		sr04[0].info.quality,  sr04[1].info.quality,
	  	sr04[0].info.distance_mm, 	sr04[1].info.distance_mm,
		errorsCounter[0], errorsCounter[1], errorsCounter[2], errorsCounter[3], 
		errorsCounter[4], errorsCounter[5], errorsCounter[6], errorsCounter[7],
		 errorsCounter[8], errorsCounter[9],  errorsCounter[10], errorsCounter[11]);
}

void getSR04Info(char sid, SR04Info& info) {
	if (sid < 0 || sid >= nsr) {
		return;
	}
	bool irq = disableInterrupts();
	info = sr04[sid].info;
	enableInterrupts(irq);
}


void stopPingImpulse() {
	//pingTimer.end();
	digitalWriteFast(sr04[currentID].trigPin, LOW);
	if (pingTimerStatus == 1) {  // we came here from usPing()
		uint8_t echo =  digitalReadFast(sr04[currentID].echoPin);
		if (echo == HIGH) { // so strange. Previous ping hasn't finished.
			pingTimerStatus = -1;
			errorsCounter[1] += 1;
			return;
		}
		
		pingTimerStatus = 2;  //   waiting for the ping to actually start
	} else {
		pingTimerStatus = 0;
		errorsCounter[2] += 1;
	}
}

FASTRUN void usEcho(char usIndex) {
	if (usIndex != currentID) { // We got the echo, but now working with different sensor..  lets skip this measurement
		errorsCounter[11] += 1;
		return;
	}
	SR04& sr = sr04[usIndex];
	uint8_t echo =  digitalReadFast(sr.echoPin);
	switch (pingTimerStatus) {
	case 2: // expecting HIGH
		if (echo == HIGH) {
			sr.echoStartTimeMks = micros();		//  when measurements started
			//sr.measurementTimeMs = millis();	//  the timestamp
			pingTimerStatus = 3;
		} else {    // measurements from ANOTHER ping STOPPED.
			pingTimerStatus = -2;
			errorsCounter[3] += 1;
		}
		break;
	case 3:  
		if (echo == LOW) {
			sr.echoStopTimeMks = micros();
			pingTimerStatus = 4; // measurement complete
			if (sr.echoStopTimeMks <= sr.echoStartTimeMks) { //  mks rollover or something strange
				//  just skip this
				errorsCounter[10] += 1;
				break;
			}
			int dt = sr.echoStopTimeMks - sr.echoStartTimeMks;
			if (dt < US_ROUNDTRIP_CM) { //   distance below 1 cm, error ?
				sr.info.measurementTimeMs = millis();
				sr.info.quality = 3;   //  bad data
				sr.info.distance_mm = (dt * 10) / US_ROUNDTRIP_CM;
				errorsCounter[4] += 1;
				break;
			}
			if (dt >= sr.maxEchoTimeMks) { //  too far away. this is not an error
				sr.info.measurementTimeMs = millis();
				sr.info.quality = 2;   //  far away ?
				sr.info.distance_mm =  (dt * 10) / US_ROUNDTRIP_CM;
				break;
			}
			// ok, we have something:
			sr.info.distance_mm = (dt * 10) / US_ROUNDTRIP_CM;
			sr.info.quality = 1;			// normal measurement
			sr.info.measurementTimeMs = millis() - ((dt / 2000));
		} else {    // ping started unexpectedly
			pingTimerStatus = -3;
			errorsCounter[9] += 1;
		}
	break;
	};
}

FASTRUN void us0Echo() {
	usEcho(0);
}

FASTRUN void us1Echo() {
	usEcho(1);
}

void SR04::srSerup(int id_, int tp, int ep, int md) {
	id = id_;
	trigPin = tp;
	echoPin = ep;
	max_cm_distance = md;
	maxEchoTimeMks = (max_cm_distance + 1) * US_ROUNDTRIP_CM;

	pinMode(echoPin, INPUT); 
	pinMode(trigPin, OUTPUT); 

	switch (id) {
	case 0:  attachInterrupt( echoPin, us0Echo, CHANGE );   break;
	case 1:  attachInterrupt( echoPin, us1Echo, CHANGE );   break;
	};
	pingErrorCounter = 0;
}

volatile char skippingPingFlag = 0;
void usPing() {
	bool irq = disableInterrupts();
		switch (pingTimerStatus) {
			case 4: break;  	//  good
			case 2:     		// ping didnt started.. 
				errorsCounter[5] += 1; 
				//enableInterrupts(irq);
				//return;
				break;
			case 3: 			// ping still in progress
				errorsCounter[6] += 1;
				switch  (skippingPingFlag) {
					case 0: 
					case 1: 
						enableInterrupts(irq);
						skippingPingFlag += 1;
						return;					
					break;
					default:;
				};

				
				break;
				
			default: 
				//xmprintf(3, "pingTimerStatus %u\r\n", pingTimerStatus);
				errorsCounter[0] += 1;
				break;
		};
		pingTimerStatus = 1;
	enableInterrupts(irq);

	if (skippingPingFlag) {
		//xmprintf(3, "skippingPingFlag(%d)\tquality: (%d, %d)  distance: [%d  %d] mm \r\n", 	skippingPingFlag,
		//sr04[0].info.quality,  sr04[1].info.quality, 	sr04[0].info.distance_mm, 	sr04[1].info.distance_mm);
		skippingPingFlag = 0;
	}
	//digitalWriteFast(sr04[currentID].trigPin, LOW);
	//delayMicroseconds(4);
	digitalWriteFast(sr04[currentID].trigPin, HIGH);
	//pingTimer.begin(stopPingImpulse, trigPulsTimeMks);
	trigTimer.trigger(10);
}

void usStartPing(char sid) {
	bool irq = disableInterrupts();
	currentID = sid;
	enableInterrupts(irq);
}

void periodicPing() {
	switch (currentID) {
	case 0: usPing(); break;
	case 1: usPing(); break;
	};
}

void usSetup() {
	sr04[0].srSerup(0, usonic0_trig_pin, usonic0_echo_pin);
	sr04[1].srSerup(1, usonic1_trig_pin, usonic1_echo_pin);
	trigTimer.begin(stopPingImpulse);
	periodicPingTimer.begin(periodicPing, 50ms);
}

