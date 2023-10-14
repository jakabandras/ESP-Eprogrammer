///////////////////////////////////////////////////////////////////////////
//
//	TFT_Menu - A simple Arduino menu class for small ST7735 TFT displays
//		using a clickable rotary encoder.
//
//	3rd Party arduino libraries used:
//
//		Bodmer's Arduino graphics library for ST7735:
//			https://github.com/Bodmer/TFT_ST7735
//
//		0xPIT's Atmel AVR C++ RotaryEncoder Implementation
//			https://github.com/0xPIT/encoder
//
//		Paul Stoffregen's TimerOne Library with optimization and expanded hardware support
//			https://github.com/PaulStoffregen/TimerOne
//
//	Code by Russ Hughes (russ@owt.com) September 2018
//
//	License:
//	This is free software. You can redistribute it and/or modify it under
//  the terms of Creative Commons Attribution 3.0 United States License.
//  To view a copy of this license, visit http://creativecommons.org/licenses/by/3.0/us/
//  or send a letter to Creative Commons, 171 Second Street, Suite 300, San Francisco, California, 94105, USA.
//
///////////////////////////////////////////////////////////////////////////

#ifndef __TFT_MENU_H__
#define __TFT_MENU_H__
#include <vector>
#include <TFT_eSPI.h>
#include <AnalogJoystick.h>
///////////////////////////////////////////////////////////////////////////
//
// The R & B color bits are reversed on my ebay green tag display
//	so these colors work for mine... YMMV.
//
///////////////////////////////////////////////////////////////////////////
#ifndef TFT_BLACK
#define TFT_BLACK 0x0000 /*   0,   0,   0 */
#define TFT_RED 0x001F	 /*   0,   0, 255 */
#define TFT_GREEN 0x07E0 /*   0, 255,   0 */
#define TFT_BLUE 0xF800	 /* 255,   0,   0 */
#define TFT_WHITE 0xFFFF /* 255, 255, 255 */
#endif					 // !TFT_BLACK

///////////////////////////////////////////////////////////////////////////
//
//	MENU structure:
//
//	option:	Value to display
//	value:	Value to return on button press
//
//  NOTE: must be defined in a function since it is stored in flash
//		using the __FlashStringHelper
//
///////////////////////////////////////////////////////////////////////////

typedef struct _MENU
{
	String option;
	int value;

} MENU;

///////////////////////////////////////////////////////////////////////////
//
//	TFT_MENU Class to display a simple menu on a TFT display
//
///////////////////////////////////////////////////////////////////////////

class TFT_MENU
{
public:
	TFT_MENU(TFT_eSPI &tft, AnalogJoystick &joystick, uint8_t textSize = 1);

	int8_t show(MENU menu[], int8_t active);

	void setColors(
		uint16_t headerForground,
		uint16_t headerBackground,
		uint16_t normalForeground,
		uint16_t normalBackground,
		uint16_t selectedForgound,
		uint16_t selectedBackground);

private:
	void printSpaces(int8_t spaces = 1);
	TFT_eSPI *tft;
	AnalogJoystick *joystick;
	int font;
	int16_t fontHeight;
	int16_t fontWidth;
	uint8_t maxChars;
	uint8_t maxLines;

	uint16_t hF = TFT_BLUE;
	uint16_t hB = TFT_WHITE;
	uint16_t nF = TFT_WHITE;
	uint16_t nB = TFT_BLUE;
	uint16_t sF = TFT_WHITE;
	uint16_t sB = TFT_RED;

};

class TFT_File : public TFT_MENU
{
public:
  TFT_File(TFT_eSPI &tft, AnalogJoystick &joystick, uint8_t textSize = 1);
	int8_t show(int8_t active);
  void setFilter(String filter);
  void refresh();
private:
	std::vector<String> items;
};


#endif // __TFT_MENU_H__