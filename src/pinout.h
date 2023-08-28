#include <Arduino.h>
#include <string>
#include <vector>

#define ADDR_OUT 2
#define ADDR_RST 3
#define DATA_OUT 4
#define DATA_RST 5
#define ENCODER_PINA     GPIO_NUM_23
#define ENCODER_PINB     GPIO_NUM_22
#define ENCODER_BTN      GPIO_NUM_21
#ifdef ESP8266
    #define LEDPIN LED_BUILTIN
#endif
// ILI9341 TFT pins
#define TFT_CS   15
#define TFT_DC   33
#define TFT_RST  32
#define TFT_MOSI 23
#define TFT_CLK  18
#define TFT_MISO 19
