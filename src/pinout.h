#include <Arduino.h>
#include <string>
#include <vector>

#define ADDR_OUT 2
#define ADDR_RST 3
#define DATA_OUT 4
#define DATA_RST 5
#define ENCODER_PINA     21
#define ENCODER_PINB     12
#define ENCODER_BTN      14
#ifdef ESP8266
    #define LEDPIN LED_BUILTIN
#endif
// ILI9341 TFT pins
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4
#define TFT_MOSI 23
#define TFT_MISO 19
#define TFT_SCLK 18
#define TFT_LED  15
