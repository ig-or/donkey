

#include "led_strip.h"
#include "ttpins.h"
#include "teetools.h"

#include <FastLED.h>

#define NUM_LEDS 30
#define PIN_CLOCK_MHZ 1  // Any setting fails.
#define LED_CHIPSET SK9822
#define PIN_DATA ledstrip_data_pin		// 26
#define PIN_CLOCK ledstrip_clock_pin 	// 27 
#define LED_ORDER BGR

CRGB leds[NUM_LEDS];

unsigned int lsShowTime = 0;
unsigned int testCounter = 0;

unsigned int lsProcessCounter = 0;
unsigned int lsProcessTime = 0;

//CLEDController ledController;
unsigned char lsModes[lsModesCount];
unsigned int lsModeLeds[lsModesCount];
unsigned int lsModeColors[lsModesCount]; 		//  0RGB 0 - 255

const unsigned int frontLeds = 15 << 10;     	//  10, 11, 12, 13, 
const unsigned int frontAngleLeds = 0x4200;		// 9 , 14
const unsigned int rearLeds = 0x20000000;		// 29

const int stopCycle = 2000;


void setupModeLeds() {
	lsModeLeds[lsFrontObstacle] =  frontLeds; 
	lsModeLeds[lsMovingForward] = frontAngleLeds;
	lsModeLeds[lsRearObstakle] = rearLeds;
	lsModeLeds[lsMovingBackward] = 0x10000000;

	lsModeColors[lsFrontObstacle] =  	0x00ff0000; 
	lsModeColors[lsMovingForward] = 	0x00d0d0d0;
	lsModeColors[lsRearObstakle] = 		0x00ff0000;
	lsModeColors[lsMovingBackward] = 	0x00d0d0d0;
}


void ledstripSetup() {
	//FastLED.addLeds<LED_CHIPSET, PIN_DATA, PIN_CLOCK, LED_ORDER, DATA_RATE_KHZ(10)>(leds, NUM_LEDS);
	int i;
	for (i = 0; i < lsModesCount; i++) {
		lsModes[i] = 0;
		lsModeLeds[i] = 0;
		lsModeColors[i] = 0;
	}
	setupModeLeds();
	FastLED.addLeds<LED_CHIPSET, PIN_DATA, PIN_CLOCK, LED_ORDER>(leds, NUM_LEDS);

	for (int i = 0; i < NUM_LEDS; ++i) {
		leds[i] = CRGB::Green;
		
		//delay(1);
		//leds[i] = CRGB::Black;
	}
	FastLED.show();
}

void ledstripTest(unsigned int now) {
	if (now < (lsShowTime + 150)) {
		return;
	}
	int x = testCounter % NUM_LEDS;
	for (int i = 0; i < NUM_LEDS; ++i) {
		if (i == x) {
				leds[i] =   0x000500; // CRGB::Green;
		} else if (i == x-1) {
			leds[i] =   0x000804; 
		} else if (i == x+1) {
			leds[i] =   0x030303; 
		} else 	{
			leds[i] = CRGB::Black;
		}
	}
	FastLED.show();
	lsShowTime = now;
	testCounter += 1;
}

void ledstripMode(LSFlashMode mode, unsigned char intensity) {
	unsigned int m = static_cast<unsigned int>(mode);
	lsModes[m] = intensity;
}

unsigned int ltmp[NUM_LEDS];

void ledstripProcess(unsigned int now) {
	if (now < (lsProcessTime + 50)) {
		return;
	}
	lsProcessTime = now;

	int i, j, b, k, kb, a;
	memset(ltmp, 0, NUM_LEDS * sizeof(unsigned int));	// black by default
	for (j = 0; j < NUM_LEDS; j++) {
		b = 1 << j;										//   led mask
		for (i = 0; i < lsModesCount; i++) {
			if (lsModes[i] == 0) {
				continue;
			}
			if (lsModeLeds[i] & b)	{					// this mode is enabled and it is controlling this led
				for (k = 2; k >= 0; k--) {
					kb = k << 3;
					a = lsModeColors[i] & (0xff << kb);	//  r, g or  b
					a >>= kb;
					a *= lsModes[i];
					a >>= 8;							// scale back
					a = (a & 0xff) << kb;
					ltmp[j] += a;
				}
				break;									//  will not consider other modes for this led
			}
		}
		//  this led is still empty..
	}

	if (lsModes[lsStop] != 0) {			//   stop mode is a spesial case
		k = now % stopCycle;	//  0 ~ stopCycle
		i = k * NUM_LEDS / stopCycle;
		if (i >= NUM_LEDS) { i = NUM_LEDS - 1; }
		if (ltmp[i] == 0) {ltmp[i] =   0x000200;}
		k = (i == 0) ? NUM_LEDS - 1 : i - 1;
		if (ltmp[k] == 0) { ltmp[k] =  0x000502; }
		k = (i == (NUM_LEDS - 1)) ? 0 : i + 1;
		if (ltmp[k] == 0) { ltmp[k] =  0x010201; }
	}

	for (j = 0; j < NUM_LEDS; j++) { 
		leds[j] = ltmp[j];
	}
	FastLED.show();
}




