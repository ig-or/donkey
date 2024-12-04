#pragma once

#include "stdint.h"
//#include "imxrt.h"
//#include "core_pins.h"

extern uint32_t msNow;
extern uint32_t mksNow;

//const int maxPWM = 65535;  // because of the 16 bit resolution
const unsigned int pwmResolution = 13;
constexpr unsigned int maxPWM = (1 << pwmResolution) - 1;  // 8191 for    13 bit

void logSetup(const char* fileName);

bool disableInterrupts();
void enableInterrupts(bool doit);
/*
struct TTime {
	uint32_t mks;
	uint32_t ms;

	///  set time to current time
	void setNow() {
		__disable_irq();
		mks = micros();
		ms = millis();
		__enable_irq();
		mks %= 1000;
	}
	int msDiff(const TTime& t1, const TTime& t2) {


	}
};
*/

/**
 *  dst    	bit0 = 1    send to USB
 * 			bit1 = 1    send to Ethernet
 * 			bit4 = 1    skip printing 'header' ( 16) 
 * */
int xmprintf(int dst, const char* s, ...);

/// @brief  enable or disable xmprintf to the Ethernet
/// @param x 
void printfToEth(bool x);

//int clamp(int v, int min, int max);
void clamp(int& v, int vmin, int vmax);



