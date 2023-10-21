#include "MCP23008.h"

MCP23008::MCP23008()
{
  address = 0x20; // Default I2C address
}

MCP23008::MCP23008(uint8_t addr)
{
  address = addr;
}

void MCP23008::begin()
{
  Wire.begin();
  // Initialize MCP23008 here
}

void MCP23008::pinMode(uint8_t pin, uint8_t mode)
{
  // Implement pinMode here
  if (pin < 8)
  {
    // Configure pin as INPUT or OUTPUT based on 'mode'
    uint8_t regValue = readRegister(IODIR);
    if (mode == OUTPUT)
    {
      regValue &= ~(1 << pin); // Set bit to 0 for OUTPUT
    }
    else
    {
      regValue |= (1 << pin); // Set bit to 1 for INPUT
    }
    writeRegister(IODIR, regValue);
  }
}

void MCP23008::digitalWrite(uint8_t pin, uint8_t value)
{
  // Implement digitalWrite here
  if (pin < 8)
  {
    // Write HIGH (1) or LOW (0) to the specified pin
    uint8_t regValue = readRegister(GPIO_);
    if (value == HIGH)
    {
      regValue |= (1 << pin); // Set bit to 1 for HIGH
    }
    else
    {
      regValue &= ~(1 << pin); // Set bit to 0 for LOW
    }
    writeRegister(GPIO_, regValue);
  }
}

uint8_t MCP23008::digitalRead(uint8_t pin)
{
  // Implement digitalRead here
  if (pin < 8)
  {
    // Read the state of the specified pin
    uint8_t regValue = readRegister(GPIO_);
    return (regValue >> pin) & 0x01;
  }
  return LOW;
}

void MCP23008::writeRegister(uint8_t reg, uint8_t value)
{
  // Implement writeRegister here
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

uint8_t MCP23008::readRegister(uint8_t reg)
{
  // Implement readRegister here
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(address, (uint8_t)1);
  if (Wire.available())
  {
    return Wire.read();
  }
  return 0;
}
