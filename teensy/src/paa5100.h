
//   https://forum.pjrc.com/threads/65138-Teensy-4-0-and-BNO08x-on-SPI2-hangs


#ifndef __PAA5100_H__
#define __PAA5100_H__

#include "Arduino.h"
#include <SPI.h>
//#include <stdint.h>

class PAA5100 {
public:
  PAA5100(SPIClass* bus, uint8_t cspin);
  boolean begin(void);
  void disable();
  void enable();

  void readMotionCount(int16_t *deltaX, int16_t *deltaY);
  void readSignalQuality(uint8_t *squal);
    
  void getMotion(int16_t *deltaX, int16_t *deltaY, uint8_t *squal);
  void getMotionSlow(int16_t *deltaX, int16_t *deltaY, uint8_t *squal);

private:
  uint8_t _cs;
  bool enabled = false;
    
  bool validateChip();
  SPISettings paa5100SPISettning = SPISettings(100000, MSBFIRST, SPI_MODE3);
  
  void registerWrite(uint8_t reg, uint8_t value);
  uint8_t registerRead(uint8_t reg);
  uint8_t  registerReadInternal(uint8_t reg);
    
  void initRegistersPAA5100JE(void);
  SPIClass* spi;
    
};

#endif //__PAA5100_H__

