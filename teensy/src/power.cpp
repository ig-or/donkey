

#include <Arduino.h>

#include "power.h"
#include "teetools.h"
#include "ttpins.h"
#include "adcsetup.h"

static constexpr float R1=220000.0f*1.0000f;
static constexpr float R2=47000.0f*1.0f;

/*
3.8v  is a 'nominal voltage'.. 
3.0 is 'very minimum
3.3 is 'minimum
4.2 is maximum
*/
static constexpr float maxVoltage = 3.9f * 4.0f * 1000.0f; ///    4.2f * 4.0f * 1000.0f;  //  4S, millivolts
static constexpr float minVoltage = 3.5f * 4.0f * 1000.0f; // 3v per cell is the very minimum.. 3.3v/cell is LOW

static constexpr float pHist = 0.5; // %
static constexpr float pVeryLow = 2.0f;  // %
static constexpr float pLow = 10.0f;
static constexpr float pOK = 125.0f;


static float vAdc = 0.0f;  //< voltage on ADC pin in millivolts
static float batteryVoltage = 0.0f; ///< millivolts
static float battPersent = 0.0f;
static PowerStatus pStatus = pwUnknown;
static OnPowerStatusChange pHandler = 0;

void batteryPrint() {
	xmprintf(0, "batt info: %.2fV (%.1f%%); vAdc=%.2fV\r\n", batteryVoltage * 0.001f, battPersent, vAdc * 0.001f);
}

PowerStatus getPowerStatus() {
	return pStatus;
}

float getBattVoltage() {
	return batteryVoltage;
}
float getBattPercent() {
	return battPersent;
}

void serPwStatusChangeHandler(OnPowerStatusChange h) {
	pHandler = h;
}

void batteryUpdate(int a) {
	//return;
	int i;
	vAdc = adcMakeMillivolts(a);

	batteryVoltage = vAdc * ((R1+R2)/R2); 
	battPersent = (batteryVoltage - minVoltage) * 100.0f/(maxVoltage - minVoltage);

	return;

	PowerStatus pPrevStatus = pStatus;
	switch (pStatus) {
		case pwUnknown:
			if (battPersent > pOK) pStatus = pwHigh;
			else if (battPersent > pLow) pStatus = pwOK;
			else if (battPersent > pVeryLow) pStatus = pwLow;
			else pStatus = pwVeryLow;
			if (pHandler) { pHandler(pPrevStatus, pStatus); }
			break;

		case pwVeryLow:
			if (battPersent > (pVeryLow + pHist))  {
				pStatus = pwLow;
				if (pHandler) { pHandler(pPrevStatus, pStatus); }
			}

		case pwLow:
			if (battPersent > (pLow + pHist)) {
				pStatus = pwOK;
				if (pHandler) { pHandler(pPrevStatus, pStatus); }
			} else if (battPersent < (pVeryLow - pHist)) { 
				pStatus = pwVeryLow;
				if (pHandler) { pHandler(pPrevStatus, pStatus); }
			}
			break;

		case pwOK:
			if (battPersent > (pOK + pHist)) {
				pStatus = pwHigh;
				if (pHandler) { pHandler(pPrevStatus, pStatus); }
			} else if (battPersent < (pLow - pHist)) { 
				pStatus = pwLow;
				if (pHandler) { pHandler(pPrevStatus, pStatus); }
			}	
			break;

		case pwHigh:
			if (battPersent < (pOK - pHist)) {
				pStatus = pwOK;
				if (pHandler) { pHandler(pPrevStatus, pStatus); }
			}
			break;
	};
}


