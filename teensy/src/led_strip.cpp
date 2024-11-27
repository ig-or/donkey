

#include "led_strip.h"
#include "ttpins.h"
#include "teetools.h"

#include <FastLED.h>

#include <cmath>

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
unsigned int lastTestTime = 0;
unsigned int lsProcessTime = 0;

unsigned char 	lsModes[lsModesCount];				///< current intensity values for all the modes
unsigned int 	lsModeLeds[lsModesCount];			///< which lEDs are participating in this modes
unsigned int 	lsModeColors[lsModesCount]; 		///<  colors for particular modes      0RGB 0 - 255

const unsigned int frontLeds = 15 << 10;     	//  10, 11, 12, 13, 
const unsigned int frontAngleLeds = 0x4200;		// 9 , 14
const unsigned int rearLeds = 0x20000000;		// 29

const int stopCycle = 2000;
int lsEyeConnectionLed = 0;
int lsManualControlLed = 1;

void setupModeLeds() {
	lsModeLeds[lsFrontObstacle] =  frontLeds; 
	lsModeLeds[lsMovingForward] = frontAngleLeds;
	lsModeLeds[lsRearObstakle] = rearLeds;
	lsModeLeds[lsMovingBackward] = 0x10000000;    	//  28
	lsModeLeds[lsEyeConnection] = 0x1;				// 0
	lsEyeConnectionLed = 0;

	lsModeColors[lsFrontObstacle] =  	0x00ff0000; 		//  red
	lsModeColors[lsMovingForward] = 	0x00d0d0d0;			// white
	lsModeColors[lsRearObstakle] = 		0x00ff0000;			// red
	lsModeColors[lsMovingBackward] = 	0x00d0d0d0;			// white
	lsModeColors[lsEyeConnection] =    0x00050808;
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
		leds[i] = 0x00000300;
	}
	leds[1] = 0x00090008;
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

void ledstripMode(LSFlashMode mode, unsigned char intensity, unsigned int color) {
	unsigned int m = static_cast<unsigned int>(mode);
	lsModes[m] = intensity;
	if (color != 0) {
		lsModeColors[m] = color;
	}
}

unsigned int ltmp[NUM_LEDS];

/**
 * @brief calculate a value for the led based on intencity and color and time.
 * 
 * @param now  current time
 * @param mode      mode (intencity)
 * @param color     color info
 * @return unsigned int  led value for the library
 */
unsigned int lsRamp(unsigned int now, int mode, unsigned int color) {
	const int ecp = 585;
	unsigned int k = now % (ecp << 1);
	float u;
	int i = mode;
	if (k <= ecp) {
		u = (k * i) / ((float)(ecp));
	} else {
		u = (i << 1) - (k * i) / ((float)(ecp));
	}
	int a = round(u);
	if (a < 0) a = 0;
	if (a > 255) a = 255;

	unsigned int c = color;
	unsigned int b = 0;
	b += ((c & 0xff) * a) >> 8;   c >>= 8;   // b
	b += (((c & 0xff) * a) >> 8) << 8;   c >>= 8;	// g
	b += (((c & 0xff) * a) >> 8) << 16;    	// r
	if (b == 0) {
		b = 1;
	}
	return b;
}

void ledstripProcess(unsigned int now) {
	if (now < (lsProcessTime + 50)) {
		return;
	}
	lsProcessTime = now;

	int i, j, b, k, kb, a;
	memset(ltmp, 0, NUM_LEDS * sizeof(unsigned int));	// black by default
	for (j = 0; j < NUM_LEDS; j++) {
		if (j == lsEyeConnectionLed) {
			continue;
		}
		if ((j == lsManualControlLed) && (lsModes[lsManualControl] != 0))
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

	const int ecp = 585;
	if (lsModes[lsManualControl] != 0) {
		ltmp[lsManualControlLed] = lsRamp(now, lsModes[lsManualControl], lsModeColors[lsManualControl]);
	}
	if (lsModes[lsEyeConnection] != 0) { //  connection: special case
		ltmp[lsEyeConnectionLed] = lsRamp(now, lsModes[lsEyeConnection], lsModeColors[lsEyeConnection]);
/*
		k = now % (ecp << 1);
		float u;
		int i = lsModes[lsEyeConnection];
		if (k <= ecp) {
			u = (k * i) / ((float)(ecp));
		} else {
			u = (i << 1) - (k * i) / ((float)(ecp));
		}
		int a = round(u);
		if (a < 0) a = 0;
		if (a > 255) a = 255;

		unsigned int c = lsModeColors[lsEyeConnection];
		unsigned int b = 0;
		b += ((c & 0xff) * a) >> 8;   c >>= 8;   // b
		b += (((c & 0xff) * a) >> 8) << 8;   c >>= 8;	// g
		b += (((c & 0xff) * a) >> 8) << 16;    	// r
		if (b == 0) {
			b = 1;
		}

		ltmp[lsEyeConnectionLed] = b;
		//if (now - lastTestTime > 1000) {
		//	lastTestTime = now;
		//	xmprintf(1, "ledstripProcess  b = %u; k = %d; i = %u, u = %.3f, a = %d, c = %u, lsModeColors = %u  \r\n", b, k, i, u, a, c,
		//		lsModeColors[lsEyeConnection]);
		//}
		*/
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
	lsProcessCounter += 1;
}




