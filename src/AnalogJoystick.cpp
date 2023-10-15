#include <Arduino.h>
#include "AnalogJoystick.h"

AnalogJoystick::AnalogJoystick(int xpin, int ypin, int bpin, int calValue) {
    XPin = xpin;
    YPin = ypin;
    BPin = bpin;
    jCalibration = calValue;
}

AnalogJoystick::AnalogJoystick(int xpin, int ypin, int bpin) {
    XPin = xpin;
    YPin = ypin;
    BPin = bpin;
    jCalibration = 2250;
}

inline int AnalogJoystick::getDirection()
{
    int direction = 0;
    int peekValue = analogRead(YPin);
    if (peekValue>(jCalibration+thereshold))
    {
        direction = 1;
    } else if (peekValue<(jCalibration-thereshold))
    {
        direction = -1;
    }
    return direction;
}