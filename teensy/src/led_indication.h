
#pragma once
#include "teetools.h"


struct LedIndication {
	enum LIMode {
		LIRamp,
		LIStop
	};
	LedIndication();
	/**
	 *  \param m mode
	 *  \param f frequency in Hz
	 * */
	void liSetMode(LIMode m, float f = 0.0);

	/**
	 * \param ms current millisecond?
	 * \return PWM value for the led
	 * */
	int liGet(unsigned int ms);
private:
	LIMode liMode = LIStop;
	static constexpr int maxPower = maxPWM * 0.8f;
	int period = 1000; ///< period in ms
	int halfPeriod = 500;
	float mph = maxPower / 500;
};

extern LedIndication led1;


