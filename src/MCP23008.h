#ifndef MCP23008_H
#define MCP23008_H

#include <Arduino.h>
#include <Wire.h>

#define IODIR     0x00
#define IPOL      0x01
#define GPINTEN   0x02
#define DEFVAL    0x03
#define INTCON    0x04
#define IOCON     0x05
#define GPPU      0x06
#define INTF      0x07
#define INTCAP    0x08  //Read only
#define GPIO_     0x09
#define OLAT      0x0a

class MCP23008
{
public:
  MCP23008();
  MCP23008(uint8_t addr);
  MCP23008(uint8_t sapin, uint8_t scpin);
  MCP23008(uint8_t addr, uint8_t sapin, uint8_t scpin);
  void begin();
  void pinMode(uint8_t pin, uint8_t mode);
  void digitalWrite(uint8_t pin, uint8_t value);
  uint8_t digitalRead(uint8_t pin);

private:
  uint8_t address;
  uint8_t sdapin, sdcpin;
  void writeRegister(uint8_t reg, uint8_t value);
  uint8_t readRegister(uint8_t reg);
};

#endif
