#ifndef MCP23008_H
#define MCP23008_H

#include <Arduino.h>
#include <Wire.h>

class MCP23008
{
public:
  MCP23008();
  void begin();
  void pinMode(uint8_t pin, uint8_t mode);
  void digitalWrite(uint8_t pin, uint8_t value);
  uint8_t digitalRead(uint8_t pin);

private:
  uint8_t address;
  void writeRegister(uint8_t reg, uint8_t value);
  uint8_t readRegister(uint8_t reg);
};

#endif
