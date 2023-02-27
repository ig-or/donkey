
#include <Arduino.h>
#include "ttpins.h"
#include "ADC.h"
#include "adcsetup.h"


ADC* adc = 0;

adcHandler adch[2] = {0, 0};

float adcMakeMillivolts(int v) {
	float vAdc = ((static_cast<float>(v))/1.0f) * a2mv; //  millivolts
	return vAdc;
}

void adc_isr1() {
	//xmprintf(0, "adc isr 1 \r\n");
	if (adch[1] == 0) {
		adc->adc1->readSingle();
		return;
	}
	adch[1]();
}
void adc_isr0() {
	//xmprintf(0, "adc isr 0 \r\n");
	if (adch[0] == 0) {
		adc->adc0->readSingle();
		return;
	}
	adch[0]();
}

void setAdcHandler(adcHandler h, int k) {
	if ((k < 0) || (k > 1)) {
		return;
	}
	adch[k] = h;
}
bool adcStartSingleRead(int a, int pin) {
	//bool test2 = adc->adc0->startSingleRead(bvPin);
	switch (a) {
	case 0: return adc->adc0->startSingleRead(pin); break;
	case 1: return adc->adc1->startSingleRead(pin); break;
	default:
	return false;
	};
	return false;
}
int adcReadSingle(int a) {  // adc->adc0->readSingle();
	switch (a) {
	case 0: return adc->adc0->readSingle(); break;
	case 1: return adc->adc1->readSingle(); break;
	default:
	return 0;
	};
	return 0;
}

void setupADC() {
	adc = new ADC();

    //analogReadResolution(adcResolution);
	adc->adc0->setResolution(adcResolution);
	adc->adc1->setResolution(adcResolution);

	adc->adc0->enableInterrupts(adc_isr0, 255);
	adc->adc1->enableInterrupts(adc_isr1, 255);

}

