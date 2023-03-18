

#include "sr04.h"
#include "ttpins.h"
#include "teetools.h"
#include "xmroundbuf.h"
#include "xmfilter.h"

#include "core_pins.h"
#include "avr/pgmspace.h" 
#include "IntervalTimer.h"
#include "TeensyTimerTool.h"
#include <numeric>
#include <iterator>
#include <atomic>
#include  <algorithm>

struct SR04 {
	int id = 0;				//  id of this device
	int trigPin;			//   trigger pin
	int echoPin;			//  echo pin
	int max_cm_distance;	// max distance in cm
	int maxEchoTimeMks = 0;	//   maximum echo time in mks
	//int minEchoTimeMks = 0;	//   minimum echo time in mks
	SR04Info info; //  measurement result
	static const int hInfoCount = 5;
	XMRoundBuf<SR04Info, hInfoCount> hInfo; // measurement results
	unsigned int echoStartTimeMks = 0;
	unsigned int echoStopTimeMks = 0;
	unsigned int pingCounter = 0;
	unsigned int echoUpCounter = 0;
	unsigned int echoDownCounter = 0;
		
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
volatile char pingStatus = 0;
std::atomic<char>   currentID;
volatile unsigned int pingErrorCounter = 0;

unsigned int errorsCounter[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void usPrint() {
	xmprintf(3, "SR04(%u  %u), (%u  %u), (%u  %u) \t quality: (%d, %d)  distance: [%d  %d] mm,  errors=[%u  %u  %u  %u  %u  %u  %u  %u  %u  %u %u  %u]\r\n",
		sr04[0].pingCounter, sr04[1].pingCounter, 
		sr04[0].echoUpCounter, sr04[1].echoUpCounter, 
		sr04[0].echoDownCounter, sr04[1].echoDownCounter, 

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
/*
int getMedianIndex(SR04Info bb[SR04::hInfoCount], int ones, int oneIndex[SR04::hInfoCount]) {
	int ii[SR04::hInfoCount];
	std::iota(std::begin(ii), std::end(ii), 0);
	std::sort(std::begin(ii), std::end(ii), [bb, oneIndex](int a, int b) {return bb[oneIndex[a]].distance_mm < bb[oneIndex[b]].distance_mm;});

}
*/
/*
void getSR04Info2(char sid, SR04Info& info) {
	if (sid < 0 || sid >= nsr) {	//  check the args
		return;
	}

	//  copy all the history into 'b'
	SR04Info b[SR04::hInfoCount];
	int i, j;
	bool irq = disableInterrupts();
		for (i = 0; i < SR04::hInfoCount; i++) {
			b[i] = sr04[sid].hInfo[SR04::hInfoCount - 1 - i]; // put newest info in the beginning of 'b'
		}
	enableInterrupts(irq);

	//  find out the indices where we have some real measurement
	char ones = 0;								// number of the good measurements
	char oneIndex[SR04::hInfoCount];			// indexes of the good measurements
	for (i = 0; i < SR04::hInfoCount; i++) {	// for all the measurements	
		if (b[i].quality == 1) {				//  if its good
			oneIndex[ones] = i;					// remember its index
			ones += 1;
		}
	}
	auto f = [=](int c, int d) -> bool {
		return b[c].distance_mm > b[d].distance_mm; 
	};
	switch (ones) {
		case 0: 								//  everything is bad
			info = b[0]; 						//just return the last measurement
			break;
		case 1: 								// only one good measurement: return it
			info = b[oneIndex[0]];
			break;
		case 2: 								// return the average of those two
			info.quality = 1;
			info.distance_mm = (b[oneIndex[0]].distance_mm + b[oneIndex[1]].distance_mm) / 2;
			info.measurementTimeMs = (b[oneIndex[0]].measurementTimeMs + b[oneIndex[1]].measurementTimeMs) / 2;
			break;
		case 3: 								//  we can calculate the median
			j = opt_med3f((int*)(oneIndex), f);
			info = b[j];
			break;
		case 4:									//  return the median from 3 LAST measurements
			j = opt_med3f((int*)(oneIndex), f);
			info = b[j];
			break;
		default:  								//  everything is good
			j = opt_med5f((int*)(oneIndex), f);
			info = b[j];
			break;
	};
}
*/

void stopPingImpulse() {
	digitalWriteFast(sr04[currentID].trigPin, LOW);
	if (pingStatus == 1) {  // we came here from usPing()
		uint8_t echo =  digitalReadFast(sr04[currentID].echoPin);
		if (echo == HIGH) { // so strange. Previous ping hasn't finished.
			pingStatus = -1;
			errorsCounter[1] += 1;
			return;
		} else {  //  this is expected
			pingStatus = 2;  //   waiting for the ping to actually start
		}
	} else {
		pingStatus = 0;
		errorsCounter[2] += 1;
	}
}

FASTRUN void usEcho(char usIndex) {
	//return;
	if (usIndex != currentID) { // We got the echo, but now working with different sensor..  lets skip this measurement
		errorsCounter[11] += 1;
		return;
	}
	//xmprintf(1, " %d,%d ", usIndex, currentID);

	SR04& sr = sr04[usIndex];
	uint8_t echo =  digitalReadFast(sr.echoPin);
	if (echo == HIGH) {
		sr.echoUpCounter += 1;
	} else {
		sr.echoDownCounter += 1;
	}
	switch (pingStatus) {
	case 2: // expecting HIGH
		if (echo == HIGH) {
			sr.echoStartTimeMks = micros();		//  when measurements started
			//sr.measurementTimeMs = millis();	//  the timestamp
			pingStatus = 3;
		} else {    // measurements from ANOTHER ping STOPPED.
			pingStatus = -2;
			errorsCounter[3] += 1;
		}
		break;
	case 3:  
		if (echo == LOW) {
			sr.echoStopTimeMks = micros();
			pingStatus = 4; // measurement complete
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
				sr.hInfo.put(sr.info);
				errorsCounter[4] += 1;
				break;
			}
			if (dt >= sr.maxEchoTimeMks) { //  too far away. this is not an error
				sr.info.measurementTimeMs = millis();
				sr.info.quality = 2;   //  far away ?
				sr.info.distance_mm =  (dt * 10) / US_ROUNDTRIP_CM;
				sr.hInfo.put(sr.info);
				break;
			}
			// ok, we have something:
			sr.info.distance_mm = (dt * 10) / US_ROUNDTRIP_CM;
			sr.info.quality = 1;			// normal measurement
			sr.info.measurementTimeMs = millis() - ((dt / 2000));
			sr.hInfo.put(sr.info);
		} else {    // ping started unexpectedly
			pingStatus = -3;
			errorsCounter[9] += 1;
		}
	break;
	};
}

FASTRUN void us0Echo() {
	//xmprintf(1, ".");
	usEcho(0);
}

FASTRUN void us1Echo() {
	usEcho(1);
}

void SR04::srSerup(int id_, int tp, int ep, int md) {
	
	id = id_;
	//return;
	trigPin = tp;
	//return;

	echoPin = ep;
	max_cm_distance = md;
	maxEchoTimeMks = (max_cm_distance + 1) * US_ROUNDTRIP_CM;

	pinMode(echoPin, INPUT); 
	pinMode(trigPin, OUTPUT); 

	pingErrorCounter = 0;
}

volatile char skippingPingFlag = 0;
void usPing() {
	//return;
	bool irq = disableInterrupts();
		switch (pingStatus) {
			case 0: break;		// start another mode..
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
				//xmprintf(3, "pingStatus %u\r\n", pingStatus);
				errorsCounter[0] += 1;
				break;
		};
		pingStatus = 1;
	enableInterrupts(irq);

	if (skippingPingFlag) {
		//xmprintf(3, "skippingPingFlag(%d)\tquality: (%d, %d)  distance: [%d  %d] mm \r\n", 	skippingPingFlag,
		//sr04[0].info.quality,  sr04[1].info.quality, 	sr04[0].info.distance_mm, 	sr04[1].info.distance_mm);
		skippingPingFlag = 0;
	}
	//digitalWriteFast(sr04[currentID].trigPin, LOW);
	//delayMicroseconds(4);
	digitalWriteFast(sr04[currentID].trigPin, HIGH);
	trigTimer.trigger(10);
	sr04[currentID].pingCounter += 1;
}

void usStartPing(char sid) {
	bool irq = disableInterrupts();
		currentID = sid;
		pingStatus = 0;
	enableInterrupts(irq);
}

void periodicPing() {
	//return;
	switch (currentID) {
	case 0: usPing(); break;
	case 1: usPing(); break;
	};
}

void usSetup() {
	xmprintf(1, "ultra sonic setup ... ..  ");
	currentID = 0;
	pingStatus = 0;
	sr04[0].srSerup(0, usonic0_trig_pin, usonic0_echo_pin);
	sr04[1].srSerup(1, usonic1_trig_pin, usonic1_echo_pin);
	attachInterrupt( usonic0_echo_pin, us0Echo, CHANGE );   
	attachInterrupt( usonic1_echo_pin, us1Echo, CHANGE );   
	xmprintf(17, ". srSerup() .. ");
	//return;


	trigTimer.begin(stopPingImpulse);
	//return;
	periodicPingTimer.begin(periodicPing, 50ms);
	xmprintf(17, ".. ... OK \r\n");
}

