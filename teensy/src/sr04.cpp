

#include "sr04.h"
#include "ttpins.h"

#include "core_pins.h"
#include "IntervalTimer.h"
#include "teetools.h"

struct SR04 {
	int id = 0;				//  id of this device
	int trigPin;			//   trigger pin
	int echoPin;			//  echo pin
	int max_cm_distance;	// max distance in cm
	int maxEchoTimeMks = 0;	//   maximum echo time in mks
	//int minEchoTimeMks = 0;	//   minimum echo time in mks

	char quality = 0;		//  0 -> quality is bad
	unsigned int measurementTimeMs = 0; //  the timestamp
	unsigned int distance_mm = 0; //  measured distance in millimeters

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

IntervalTimer pingTimer;//  this is for trigger pulse generation (kind of overkill)
volatile char pingTimerStatus = 0;
volatile unsigned int pingErrorCounter = 0;


void usPrint() {
	xmprintf(3, "SR04\t%d  %d    %u   \r\n",sr04[0].quality,  sr04[0].distance_mm,  pingErrorCounter);
}


void stopPingImpulse() {
	pingTimer.end();
	if (pingTimerStatus == 1) {  // we came here from usPing()
		uint8_t echo =  digitalReadFast(sr04[0].echoPin);
		if (echo == HIGH) { // so strange
			pingTimerStatus = -1;
			return;
		}
		digitalWriteFast(sr04[0].trigPin, LOW);
		pingTimerStatus = 2;
	} else {
		pingTimerStatus = 0;
	}
}

void usEcho(int usIndex) {
	SR04& sr = sr04[usIndex];
	uint8_t echo =  digitalReadFast(sr.echoPin);
	switch (pingTimerStatus) {
	case 2: 
		if (echo == HIGH) {
			sr.echoStartTimeMks = micros();		//  when measurements started
			//sr.measurementTimeMs = millis();	//  the timestamp
			pingTimerStatus = 3;
		} else {
			pingTimerStatus = -2;
		}
		break;
	case 3:  
		if (echo == LOW) {
			sr.echoStopTimeMks = micros();
			pingTimerStatus = 4; // measurement complete
			if (sr.echoStopTimeMks <= sr.echoStartTimeMks) { //  mks rollover or something strange
				//  just skip this
				break;
			}
			int dt = sr.echoStopTimeMks - sr.echoStartTimeMks;
			if (dt < US_ROUNDTRIP_CM) { //   distance below 1 cm, error ?
				sr.measurementTimeMs = millis();
				sr.quality = 3;   //  bad data
				sr.distance_mm = (dt * 10) / US_ROUNDTRIP_CM;
				break;
			}
			if (dt >= sr.maxEchoTimeMks) { //  too far away
				sr.measurementTimeMs = millis();
				sr.quality = 2;   //  far away ?
				sr.distance_mm = 0;	
				break;
			}
			// ok, we have something:
			sr.distance_mm = (dt * 10) / US_ROUNDTRIP_CM;
			sr.quality = 1;			// normal measurement
			sr.measurementTimeMs = millis() - ((dt / 2000));
		} else {
			pingTimerStatus = -3;
		}
	break;
	};
}

void us0Echo() {
	usEcho(0);
}

void us1Echo() {
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

void usPing() {
	digitalWriteFast(sr04[0].trigPin, HIGH);
	bool irq = disableInterrupts();
	if (pingTimerStatus != 4) {
		pingErrorCounter += 1;
	}
	pingTimerStatus = 1;
	enableInterrupts(irq);
	pingTimer.begin(stopPingImpulse, trigPulsTimeMks);
}

void usSetup() {
	sr04[0].srSerup(0, usonic0_trig_pin, usonic0_echo_pin);
	sr04[1].srSerup(1, usonic1_trig_pin, usonic1_echo_pin);
}

