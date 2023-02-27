
#pragma once

const unsigned int adcResolution = 10;
constexpr unsigned int maxADC = (1 << adcResolution)  - 1;
constexpr float a2mv = 3300.0f / maxADC; // translation from adc reading into millivolts; like 3.3v is a full range
constexpr float a2ma = 50 * 3300.0f / maxADC; // translation from adc reading into milliamps for motor driver; 50 for every millivolt

//extern ADC* adc;
typedef void(*adcHandler)(void);

/** starts the single read procedure.
 * \param a ADC #
 * \param pin pin number
 * 
*/
bool adcStartSingleRead(int a, int pin);
/**
 * \param a ADC #

 * 
*/
int adcReadSingle(int a);
void setupADC();
void setAdcHandler(adcHandler h, int k);

/**
 * make millivolts from ADC value.
*/
float adcMakeMillivolts(int v);
