

#include "led_strip.h"
#include "ttpins.h"

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

//CLEDController ledController;

void ledstripSetup() {
	//FastLED.addLeds<LED_CHIPSET, PIN_DATA, PIN_CLOCK, LED_ORDER, DATA_RATE_KHZ(10)>(leds, NUM_LEDS);
	FastLED.addLeds<LED_CHIPSET, PIN_DATA, PIN_CLOCK, LED_ORDER>(leds, NUM_LEDS);

	for (int i = 0; i < NUM_LEDS; ++i) {
		leds[i] = CRGB::Green;
		
		//delay(1);
		//leds[i] = CRGB::Black;
	}
	FastLED.show();
}

void ledstripTest(unsigned int now) {
	if (now < (lsShowTime + 100)) {
		return;
	}
	int x = testCounter % NUM_LEDS;
	for (int i = 0; i < NUM_LEDS; ++i) {
		if (i == x) {
			leds[i] = CRGB::Green;
		}else {
			leds[i] = CRGB::Black;
		}
	}
	FastLED.show();
	lsShowTime = now;
	testCounter += 1;
}

