#include "TFT_Menu.h"

///////////////////////////////////////////////////////////////////////////
//
//	TFT_MENU constuctor
//
//	tft: 		tft object
//	encoder: 	ClickEncoder object
//	menuFont:	Font to use for the menu, defaults to 1
//	textSize:	Size of text to use for the menu, defaults to 1
//
///////////////////////////////////////////////////////////////////////////

TFT_MENU::TFT_MENU(TFT_eSPI &tfts, AnalogJoystick &joystick, uint8_t textSize)
{
	this->tft = &tfts;
	this->joystick = &joystick;
	fontHeight = tft->gFont.yAdvance * textSize;
	fontWidth = tft->gFont.spaceWidth * textSize;
	maxChars = tft->width() / fontWidth;
	maxLines = tft->height() / fontHeight;
}

///////////////////////////////////////////////////////////////////////////
//
// show: Show the menu[]
//
// Parameters:
//
// 		menu[]:	Array of MENU structures containing the menu
//
//			The First entry's option will be shown as the menus Title and is not selectable.
//			The Last entry's option must be NULL.
//
//		active: Active menu option at start, defaults to the first
//
// Returns:
//		the current option when the button was held, clicked or doubleclicked.
//
//	Note:
//		use getButton to determine how the button was clicked.
//
///////////////////////////////////////////////////////////////////////////

int8_t TFT_MENU::show(MENU menu[], int8_t active = 1)
{

	uint8_t menuCount = 0;
	uint8_t first = 1;
	uint8_t lfirst = 0;
	uint8_t count = 0;
	uint8_t line = 0;
	uint8_t current = active;
	uint8_t lCurrent = 0;
	uint8_t i = 0;
	uint8_t j = 0;
	boolean again = true;

	// adjust first if the current option would be
	// off the screen

	if (current > maxLines - 1)
		first = current - maxLines + 2;

	// Clear screen

	tft->fillScreen(nB);

	// Print the menu header centered on the first line

	tft->setTextColor(hF, hB);
	uint8_t len = menu[0].option.length();

	if (len < maxChars)
		i = ((maxChars / 2) - (len / 2)) * fontWidth;
	else
		i = 1;

	tft->fillRect(0, 0, 127, fontHeight, hB);
	tft->setCursor(i, 0);
	tft->println(menu[0].option);

	// determine the number of items in the menu

	i = 0;
	while (menu[i++].option)
		menuCount++;

	// display the menu

	tft->setTextColor(nF, nB);

	do
	{
		if (current != lCurrent || first != lfirst)
		{								   // if current or first has changed draw the menu
			tft->setCursor(0, fontHeight); // skip the first display line (the title)
			lCurrent = current;
			lfirst = first;
			for (i = 0; i < maxLines - 1; i++)
			{ // for as many lines that wil fit on the display
				j = first + i;
				if (j < menuCount)
				{
					if (j == current)
					{
						tft->setTextColor(sF, sB);
						tft->print(F(">"));
						tft->setTextColor(nF, nB);
						tft->print(menu[j].option);
					}
					else
					{
						printSpaces(1);
						tft->print(menu[j].option);
					}

					printSpaces(maxChars - menu[j].option.length());
					tft->println();
				}
			}
		}

		switch (joystick->getValue())
		{
		case 0:
			// button = encoder->getButton();
			if (joystick->isButtonPressed())
				again = false;
			break;

		case 1:
			if (current > 1)
			{
				current--;
				if (current < first)
					first--;
			}
			break;

		case -1:
			if (current < menuCount - 1)
			{
				current++;
				if (current > maxLines - 1)
					first = current - maxLines + 2;
			}
			break;
		}
	} while (again);

	return menu[current].value;
}

///////////////////////////////////////////////////////////////////////////
//
//	setColors: Set the text colors used by the menu
//
///////////////////////////////////////////////////////////////////////////

void TFT_MENU::setColors(
	uint16_t headerForground,
	uint16_t headerBackground,
	uint16_t normalForeground,
	uint16_t normalBackground,
	uint16_t selectedForgound,
	uint16_t selectedBackground)
{
	hF = headerForground;
	hB = headerBackground;
	nF = normalForeground;
	nB = normalBackground;
	sF = selectedForgound;
	sB = selectedBackground;
}

///////////////////////////////////////////////////////////////////////////
//
//	printSpaces: Prints spaces on TFT, used to clear to the end of the line
//
///////////////////////////////////////////////////////////////////////////

void TFT_MENU::printSpaces(int8_t spaces)
{
	if (spaces > 0)
		for (int i = 0; i < spaces; i++)
			tft->print(F(" "));
}

TFT_File::TFT_File(TFT_eSPI &tfts, AnalogJoystick &joystick, uint8_t textSize):TFT_MENU(tfts,joystick,textSize)
{
}

int8_t TFT_File::show(int8_t active)
{
    return 0;
}