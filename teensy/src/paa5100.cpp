

#include "paa5100.h"

#define DEBUGPAA5100 1

PAA5100::PAA5100(SPIClass* bus, uint8_t cspin)
{
    spi = bus;
    _cs = cspin;
    pinMode(_cs, OUTPUT);
    digitalWriteFast(_cs, HIGH);
}

void PAA5100::enable() {
    enabled = true;
}

void PAA5100::disable() {
    enabled = false;
    digitalWriteFast(_cs, HIGH);
}

boolean PAA5100::begin(void) {
  spi->begin();
  spi->beginTransaction(paa5100SPISettning);

#ifdef DEBUGPAA5100
    Serial.println("Debug point 0");
#endif

  // Make sure the SPI bus is reset
  digitalWriteFast(_cs, LOW);
  delay(1);
  digitalWriteFast(_cs, HIGH);
  spi->endTransaction();
    
  // Power on reset
#ifdef DEBUGPAA5100
    Serial.println("Debug point 02");
#endif
  delay(1);
  registerWrite(0x3A, 0x5A);
  delay(5);
#ifdef DEBUGPAA5100
    Serial.println("Debug point 03");
#endif
    
   // Test the SPI communication, checking chipId and inverse chipId
   if (validateChip()) {
        registerRead(0x02);
        registerRead(0x03);
        registerRead(0x04);
        registerRead(0x05);
        registerRead(0x06);
#ifdef DEBUGPAA5100
    Serial.println("Debug point 3");
#endif
        delay(1);
        initRegistersPAA5100JE();
#ifdef DEBUGPAA5100
    Serial.println("Debug point 4");
#endif

    }
    else return false;
    
  enable();
  return true;
}

bool PAA5100::validateChip() {
    
#ifdef DEBUGPAA5100
    Serial.println("Debug point 1");
#endif
    
    uint8_t chipId = registerRead(0x00);
    uint8_t dIpihc = registerRead(0x5F);

#ifdef DEBUGPAA5100
    Serial.println("Debug point 2");
#endif
    
    if (chipId != 0x49 && dIpihc != 0xB8) {
        Serial.print("Optical flow sensor initialization error: chip ID =");
        Serial.print(chipId);
        Serial.print(", dIpihc ID =");
        Serial.println(dIpihc);
        return false;
    }
    else {
        return true;
    }
}

// Functional access

void PAA5100::readMotionCount(int16_t *deltaX, int16_t *deltaY)
{
  registerRead(0x02);
  *deltaX = ((int16_t)registerRead(0x04) << 8) | registerRead(0x03);
  *deltaY = ((int16_t)registerRead(0x06) << 8) | registerRead(0x05);
#ifdef DEBUGPAA5100
    Serial.println("Debug point 5");
#endif
}

void PAA5100::readSignalQuality(uint8_t *squal)
{
  *squal = (uint8_t)registerRead(0x07);
}

void PAA5100::getMotionSlow(int16_t *deltaX, int16_t *deltaY, uint8_t *squal) {
    registerRead(0x02);
    *deltaX = ((int16_t)registerRead(0x04) << 8) | registerRead(0x03);
    *deltaY = ((int16_t)registerRead(0x06) << 8) | registerRead(0x05);
    *squal = (uint8_t)registerRead(0x07);
}

void PAA5100::getMotion(int16_t *deltaX, int16_t *deltaY, uint8_t *squal) {
    if (enabled) {
        spi->beginTransaction(paa5100SPISettning);
        digitalWriteFast(_cs, LOW);
        //delayMicroseconds(2);
        delayNanoseconds(200);
        
#ifdef DEBUGPAA5100
    Serial.println("Debug point 6");
#endif

        registerReadInternal(0x02);
        *deltaX = ((int16_t)registerReadInternal(0x04) << 8) | registerReadInternal(0x03);
        *deltaY = ((int16_t)registerReadInternal(0x06) << 8) | registerReadInternal(0x05);
        *squal = (uint8_t)registerReadInternal(0x07);
        //delayMicroseconds(2);
        delayNanoseconds(200);
        
#ifdef DEBUGPAA5100
    Serial.println("Debug point 7");
#endif

        digitalWriteFast(_cs, HIGH);
        spi->endTransaction();
    }
    else {
        *deltaX = 0;
        *deltaY = 0;
        *squal = 0;
    }
}

// Low level register access
void PAA5100::registerWrite(uint8_t reg, uint8_t value) {
  reg |= 0x80u;

  spi->beginTransaction(paa5100SPISettning);
  digitalWriteFast(_cs, LOW);
  delayMicroseconds(2);
#ifdef DEBUGPAA5100
    Serial.println("Debug point registerWrite 1");
#endif
  spi->transfer(reg);
#ifdef DEBUGPAA5100
    Serial.println("Debug point registerWrite 2");
#endif
  spi->transfer(value);
#ifdef DEBUGPAA5100
    Serial.println("Debug point registerWrite 3");
#endif
  //delayMicroseconds(2);
  delayNanoseconds(200);
  digitalWriteFast(_cs, HIGH);
  spi->endTransaction();
#ifdef DEBUGPAA5100
    Serial.println("Debug point registerWrite 4");
#endif
}

uint8_t PAA5100::registerReadInternal(uint8_t reg) {
    reg &= ~0x80u;
    spi->transfer(reg);
    uint8_t value = spi->transfer(0);
    return value;
}

uint8_t PAA5100::registerRead(uint8_t reg) {
  reg &= ~0x80u;

  spi->beginTransaction(paa5100SPISettning);
  digitalWriteFast(_cs, LOW);
  delayNanoseconds(200);
  //delayMicroseconds(2);
  spi->transfer(reg);
  uint8_t value = spi->transfer(0);
  //delayMicroseconds(2);
  delayNanoseconds(200);
  digitalWriteFast(_cs, HIGH);

  spi->endTransaction();
  return value;
}

void PAA5100::initRegistersPAA5100JE()
{
    registerWrite(0x7f, 0x00);
    registerWrite(0x55, 0x01);
    registerWrite(0x50, 0x07);
    registerWrite(0x7f, 0x0e);
    registerWrite(0x43, 0x10);
    
    if(registerRead(0x67) & 0b10000000) registerWrite(0x48, 0x04);
    else registerWrite(0x48, 0x02);
    
    registerWrite(0x7f, 0x00);
    registerWrite(0x51, 0x7b);
    registerWrite(0x50, 0x00);
    registerWrite(0x55, 0x00);
    registerWrite(0x7f, 0x0e);
    
    
    if(registerRead(0x73) == 0x00)  {
        uint8_t c1 = registerRead(0x70);
        uint8_t c2 = registerRead(0x71);
        if(c1 <= 28) c1 += 14;
        if(c1 > 28) c1 += 11;
        c1 = max(0,min(0x3F, c1));
        c2 = (c2 * 45);
        
        registerWrite(0x7f, 0x00);
        registerWrite(0x61, 0xad);
        registerWrite(0x51, 0x70);
        registerWrite(0x7f, 0x0e);
        
        registerWrite(0x70, c1);
        registerWrite(0x71, c2);
    }
    
    
    registerWrite(0x7f, 0x00);
    registerWrite(0x61, 0xad);
    registerWrite(0x7f, 0x03);
    registerWrite(0x40, 0x00);
    registerWrite(0x7f, 0x05);
    registerWrite(0x41, 0xb3);
    registerWrite(0x43, 0xf1);
    registerWrite(0x45, 0x14);
    registerWrite(0x5f, 0x34);
    registerWrite(0x7b, 0x08);
    registerWrite(0x5e, 0x34);
    registerWrite(0x5b, 0x11);
    registerWrite(0x6d, 0x11);
    registerWrite(0x45, 0x17);
    registerWrite(0x70, 0xe5);
    registerWrite(0x71, 0xe5);
    registerWrite(0x7f, 0x06);
    registerWrite(0x44, 0x1b);
    registerWrite(0x40, 0xbf);
    registerWrite(0x4e, 0x3f);
    registerWrite(0x7f, 0x08);
    registerWrite(0x66, 0x44);
    registerWrite(0x65, 0x20);
    registerWrite(0x6a, 0x3a);
    registerWrite(0x61, 0x05);
    registerWrite(0x62, 0x05);
    registerWrite(0x7f, 0x09);
    registerWrite(0x4f, 0xaf);
    registerWrite(0x5f, 0x40);
    registerWrite(0x48, 0x80);
    registerWrite(0x49, 0x80);
    registerWrite(0x57, 0x77);
    registerWrite(0x60, 0x78);
    registerWrite(0x61, 0x78);
    registerWrite(0x62, 0x08);
    registerWrite(0x63, 0x50);
    registerWrite(0x7f, 0x0a);
    registerWrite(0x45, 0x60);
    registerWrite(0x7f, 0x00);
    registerWrite(0x4d, 0x11);
    registerWrite(0x55, 0x80);
    registerWrite(0x74, 0x21);
    registerWrite(0x75, 0x1f);
    registerWrite(0x4a, 0x78);
    registerWrite(0x4b, 0x78);
    registerWrite(0x44, 0x08);
    registerWrite(0x45, 0x50);
    registerWrite(0x64, 0xff);
    registerWrite(0x65, 0x1f);
    registerWrite(0x7f, 0x14);
    registerWrite(0x65, 0x67);
    registerWrite(0x66, 0x08);
    registerWrite(0x63, 0x70);
    registerWrite(0x6f, 0x1c);
    registerWrite(0x7f, 0x15);
    registerWrite(0x48, 0x48);
    registerWrite(0x7f, 0x07);
    registerWrite(0x41, 0x0d);
    registerWrite(0x43, 0x14);
    registerWrite(0x4b, 0x0e);
    registerWrite(0x45, 0x0f);
    registerWrite(0x44, 0x42);
    registerWrite(0x4c, 0x80);
    registerWrite(0x7f, 0x10);
    registerWrite(0x5b, 0x02);
    registerWrite(0x7f, 0x07);
    registerWrite(0x40, 0x41);

    delay(10);
    registerWrite(0x7f, 0x00);
    registerWrite(0x32, 0x00);
    registerWrite(0x7f, 0x07);
    registerWrite(0x40, 0x40);
    registerWrite(0x7f, 0x06);
    registerWrite(0x68, 0xf0);
    registerWrite(0x69, 0x00);
    registerWrite(0x7f, 0x0d);
    registerWrite(0x48, 0xc0);
    registerWrite(0x6f, 0xd5);
    registerWrite(0x7f, 0x00);
    registerWrite(0x5b, 0xa0);
    registerWrite(0x4e, 0xa8);
    registerWrite(0x5a, 0x90);
    registerWrite(0x40, 0x80);
    registerWrite(0x73, 0x1f);

    delay(10);
    registerWrite(0x73, 0x00);
}

