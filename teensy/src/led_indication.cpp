

#include "led_indication.h"
#include <cmath>

LedIndication::LedIndication() {

}
void LedIndication::liSetMode(LIMode m, float f) {
	liMode = m;
	if (f < 0.05f) {
		f = 1.0f;
	}
	period = std::ceil(1000.0f / f);
	halfPeriod = period >> 1;
	mph = maxPower / halfPeriod;
}

int LedIndication::liGet(unsigned int ms) {
	int phase = (ms % period);
	int p = 0;
	switch (liMode) {
		case LIStop:
			break;
		case LIRamp:
			if (phase < halfPeriod) {
				p = phase * mph;
			} else {
				p = maxPower - (phase - halfPeriod)*mph;
			}
			if ( p > maxPower) {
					p = maxPower;
			}
			break;
	};
	return p;
}
LedIndication led1;

