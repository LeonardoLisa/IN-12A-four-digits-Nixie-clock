/*
	Nixie clock firmware
	Copyright (C) 2023 Leonardo Lisa

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.
	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
	USA.
*/

#include <avr/io.h>
#include <avr/interrupt.h>

#include "SK6812.h"
#include "ds3231.h"
#include "rainbow_table.h"

#define DIGITS static_cast<uint8_t>(4)
#define NO_DIGIT 10

#define CLK 4
#define DATA 5
#define LATCH 8
#define LED_DATA 13 // must be pin 13

#define PB_PLUS A0
#define PB_MENU A1
#define PB_MINUS A2

#define DEBOUNCING_t static_cast<unsigned int>(200)
#define REFRESH static_cast<unsigned long>(1000)

#define TIME_mode 0
#define TEMP_mode 1

#define UPPER true
#define LOWER false

#define SOLIDCOLOR 0 // NO_COLOR is colour 360
#define RAINBOW 1

#define H24 true
#define H12 false


struct Settings
{
	uint8_t mode, colourMode, wakeUpClk, gNightClk;
	uint16_t colour;
	bool timeFormat;
} user_settings;

void writeDisplay();
void clockBegin();
void chkDisplay();
void chkLed();
uint8_t getdata(uint8_t _max, bool _digits);
void menu();
void rainbowMode(bool var);
void slot(uint8_t hour);
void blink(uint8_t _h, uint8_t _m, uint16_t _colour);

uint8_t displayBuffer[DIGITS];
unsigned long lastMillis = 0;
rgb_colour colours[DIGITS];
volatile uint16_t rgb = 0;
uint8_t count_down = 0;

void setup()
{
	clockBegin();
	/*
	for (uint8_t i = 0; i < 25; i++)
	{
		slot(i);
		delay(1000);
	}
	*/
	chkLed();
	chkDisplay();
}

void loop()
{
	Time _T;
	int _temperature;
	uint8_t flag = 0;

	if (digitalRead(PB_PLUS) == 0)
	{
		user_settings.mode = TIME_mode;
		// Force display to update.
		flag = 1;
		// Checks if is time to go to bed.
		ds3231_get_Time(&_T);
		if (!((_T.h >= user_settings.wakeUpClk) && (_T.h < user_settings.gNightClk)))
		{
			count_down = 60;
		}
		// Button debouncing.
		delay(DEBOUNCING_t);
	}

	if (digitalRead(PB_MENU) == 0)
	{
		// Button debouncing.
		delay(DEBOUNCING_t);
		// Menu.
		menu();
		ds3231_get_Time(&_T);
		if (!((_T.h >= user_settings.wakeUpClk) && (_T.h < user_settings.gNightClk)))
		{
			count_down = 60;
		}
		// Force display to update.
		flag = 1;
	}

	if (digitalRead(PB_MINUS) == 0)
	{
		user_settings.mode = TEMP_mode;
		// Force display to update.
		flag = 1;
		// Checks if is time to go to bed.
		ds3231_get_Time(&_T);
		if (!((_T.h >= user_settings.wakeUpClk) && (_T.h < user_settings.gNightClk)))
		{
			count_down = 60;
		}
		// Button debouncing.
		delay(DEBOUNCING_t);
	}

	if (millis() - lastMillis > REFRESH || flag == 1)
	{
		lastMillis = millis();
		ds3231_get_Time(&_T);
		// Checks if is time to go to bed.
		if (flag == 1 || ((_T.h >= user_settings.wakeUpClk) && (_T.h < user_settings.gNightClk)) || count_down != 0)
		{
			// Update colors
			if (user_settings.colourMode == RAINBOW)
			{
				rainbowMode(true);
			}
			else
			{
				rainbowMode(false);
				colours[0] = rainbow[user_settings.colour];
				colours[1] = rainbow[user_settings.colour];
				colours[2] = rainbow[user_settings.colour];
				colours[3] = rainbow[user_settings.colour];
				led_strip_write(colours, DIGITS);
			}

			// Update dispay
			int aint = user_settings.mode;
			if (count_down != 0)
			{
				count_down--;
			}
			switch (aint)
			{
			case TIME_mode:
				ds3231_get_Time(&_T);

				// Acoid cathode poisoning! Clock slotting every 1h. XX:59->YY:00
				if ((displayBuffer[1]) == 5 && (displayBuffer[0] == 9) && (((_T.m / 10) % 10) == 0) && ((_T.m % 10) == 0))
				{
					if (user_settings.timeFormat)
					{
						slot(_T.h);
					}
					else
					{
						if (_T.h >= 12)
						{
							slot(_T.h - 12);
						}
						else
						{
							slot(_T.h);
						}
					}
				}

				displayBuffer[0] = _T.m % 10;
				displayBuffer[1] = (_T.m / 10) % 10;
				// Select 12-24 time format.
				if (user_settings.timeFormat)
				{
					displayBuffer[2] = _T.h % 10;
					displayBuffer[3] = (_T.h / 10) % 10;
				}
				else
				{
					if (_T.h >= 12)
					{
						_T.h -= 12;
					}
					displayBuffer[2] = _T.h % 10;
					displayBuffer[3] = (_T.h / 10) % 10;
				}
				writeDisplay();
				break;
			case TEMP_mode:
				_temperature = ds3231_temp();
				displayBuffer[0] = _temperature % 10;
				displayBuffer[1] = (_temperature / 10) % 10;
				displayBuffer[2] = (_temperature / 100) % 10;
				displayBuffer[3] = (_temperature / 1000) % 10;
				writeDisplay();
				break;
			default:
				user_settings.mode = TIME_mode;
				break;
			}
		}
		else
		{
			// Clock display off for night.
			displayBuffer[0] = NO_DIGIT;
			displayBuffer[1] = NO_DIGIT;
			displayBuffer[2] = NO_DIGIT;
			displayBuffer[3] = NO_DIGIT;
			writeDisplay();

			rainbowMode(false);
			colours[0] = rainbow[NO_COLOR];
			colours[1] = rainbow[NO_COLOR];
			colours[2] = rainbow[NO_COLOR];
			colours[3] = rainbow[NO_COLOR];
			led_strip_write(colours, DIGITS);
		}
	}
}

void clockBegin()
{
	pinMode(PB_PLUS, INPUT_PULLUP);
	pinMode(PB_MENU, INPUT_PULLUP);
	pinMode(PB_MINUS, INPUT_PULLUP);

	pinMode(CLK, OUTPUT);
	pinMode(DATA, OUTPUT);
	pinMode(LATCH, OUTPUT);
	digitalWrite(CLK, LOW);
	digitalWrite(DATA, HIGH);
	digitalWrite(LATCH, LOW);

	pinMode(LED_DATA, OUTPUT);

	ds3231_init();

	sei();
	/* Rainbow Timer interrupt */
	// CTC mode. In CTC mode the counter is cleared to zero when the counter value(TCNT2) matches the OCR2A.
	// Prescaler N = 1024
	TCCR2A |= 0x02; // 0000 0010
	TCCR2B |= 0x07; // 0000 0111
	OCR2A = 250;	// FCPU / 2 * N *(OCRA + 1)
	cli();

	user_settings.mode = TIME_mode;
	user_settings.colour = NO_COLOR;
	user_settings.colourMode = SOLIDCOLOR;
	rainbowMode(false);
	user_settings.timeFormat = H24;
	user_settings.wakeUpClk = 7;
	user_settings.gNightClk = 23;
}

void writeDisplay()
{
	digitalWrite(DATA, HIGH);
	cli();
	for (uint8_t i = 0; i < DIGITS; ++i)
	{
		if (displayBuffer[i] > 10)
		{
			displayBuffer[i] = 9;
		}
		for (uint8_t j = 10; j > 0; --j)
		{
			if (displayBuffer[i] == j && displayBuffer[i] != 10)
			{
				digitalWrite(DATA, LOW);
			}
			if (j == 10 && displayBuffer[i] == 0)
			{
				digitalWrite(DATA, LOW);
			}
			asm volatile("nop\n\t" ::); // Add 65,5ns delay.
			digitalWrite(CLK, HIGH);
			asm volatile("nop\n\t" ::); // Add 65,5ns delay.
			digitalWrite(CLK, LOW);
			digitalWrite(DATA, HIGH);
		}
	}

	asm volatile("nop\n\t" ::); // Add 65,5ns delay.
	digitalWrite(LATCH, HIGH);
	asm volatile("nop\n\t" ::); // Add 65,5ns delay.
	asm volatile("nop\n\t" ::); // Add 65,5ns delay.
	digitalWrite(LATCH, LOW);
	sei();
}

void chkDisplay()
{
	for (uint8_t i = 0; i < 11; i++)
	{
		displayBuffer[0] = i;
		displayBuffer[1] = i;
		displayBuffer[2] = i;
		displayBuffer[3] = i;
		writeDisplay();
		delay(250);
	}
}

void chkLed()
{
	for (size_t j = 0; j < 361; j++)
	{
		for (uint8_t i = 0; i < DIGITS; ++i)
		{
			colours[i] = rainbow[j];
		}
		led_strip_write(colours, DIGITS);
		delay(20);
	}
}

uint8_t getdata(uint8_t _max, bool _digits)
{
	uint8_t _var = 0;

	while (1)
	{
		// Increment _var.
		if (digitalRead(PB_PLUS) == 0)
		{
			if (_var == _max)
			{
				_var = 0;
			}
			else
			{
				_var++;
			}
			// Print data on display.
			if (_digits)
			{
				// Upper 2 digits.
				displayBuffer[3] = (_var / 10) % 10;
				displayBuffer[2] = _var % 10;
				writeDisplay();
			}
			else
			{
				// Lower 2 digits.
				displayBuffer[1] = (_var / 10) % 10;
				displayBuffer[0] = _var % 10;
				writeDisplay();
			}
			delay(DEBOUNCING_t);
		}

		// Decrement _var.
		if (digitalRead(PB_MINUS) == 0)
		{
			if (_var == 0)
			{
				_var = _max;
			}
			else
			{
				_var--;
			}
			// Print data on display.
			if (_digits)
			{
				// Upper 2 digits.
				displayBuffer[3] = (_var / 10) % 10;
				displayBuffer[2] = _var % 10;
				writeDisplay();
			}
			else
			{
				// Lower 2 digits.
				displayBuffer[1] = (_var / 10) % 10;
				displayBuffer[0] = _var % 10;
				writeDisplay();
			}
			delay(DEBOUNCING_t);
		}

		// Confirm input data.
		if (digitalRead(PB_MENU) == 0)
		{
			delay(DEBOUNCING_t);
			return _var;
		}
	}
}

void menu()
{
	uint8_t menu_index = 0;
	Time _t;

	// Disable RGB leds.
	rainbowMode(false);
	colours[0] = rainbow[NO_COLOR];
	colours[1] = rainbow[NO_COLOR];
	colours[2] = rainbow[NO_COLOR];
	colours[3] = rainbow[NO_COLOR];
	led_strip_write(colours, DIGITS);

	// Set display digits.
	displayBuffer[0] = menu_index;
	displayBuffer[1] = NO_DIGIT;
	displayBuffer[2] = NO_DIGIT;
	displayBuffer[3] = NO_DIGIT;
	writeDisplay();

	while (1)
	{
		delay(DEBOUNCING_t);

		// Increment menu index.
		if (digitalRead(PB_PLUS) == 0)
		{
			if (menu_index == 3)
			{
				menu_index = 0;
			}
			else
			{
				menu_index++;
			}
			displayBuffer[0] = menu_index;
			writeDisplay();
		}

		// Decrement menu index.
		if (digitalRead(PB_MINUS) == 0)
		{
			if (menu_index == 0)
			{
				menu_index = 3;
			}
			else
			{
				menu_index--;
			}
			displayBuffer[0] = menu_index;
			writeDisplay();
		}

		// Open sub-menu.
		if (digitalRead(PB_MENU) == 0)
		{
			delay(DEBOUNCING_t);
			switch (menu_index)
			{
			case 0:
				/* Set Time */
				displayBuffer[0] = 0;
				displayBuffer[1] = 0;
				displayBuffer[2] = 0;
				displayBuffer[3] = 0;
				writeDisplay();
				colours[0] = rainbow[128];
				colours[1] = rainbow[128];
				led_strip_write(colours, DIGITS);
				_t.h = getdata(23, UPPER);
				colours[0] = rainbow[NO_COLOR];
				colours[1] = rainbow[NO_COLOR];
				colours[2] = rainbow[128];
				colours[3] = rainbow[128];
				led_strip_write(colours, DIGITS);
				delay(DEBOUNCING_t + 50);
				_t.m = getdata(59, LOWER);
				colours[2] = rainbow[NO_COLOR];
				colours[3] = rainbow[NO_COLOR];
				led_strip_write(colours, DIGITS);
				delay(DEBOUNCING_t + 50);
				_t.s = 0;
				// Ds3231 Update time.
				ds3231_update(&_t);
				/* Select 24h or 12h format */
				displayBuffer[0] = 4;
				displayBuffer[1] = 2;
				displayBuffer[2] = 0;
				displayBuffer[3] = 0;
				writeDisplay();
				user_settings.timeFormat = H24;
				while (digitalRead(PB_MENU) == 1)
				{
					if (digitalRead(PB_MINUS) == 0)
					{
						displayBuffer[0] = 2;
						displayBuffer[1] = 1;
						writeDisplay();
						user_settings.timeFormat = H12;
						delay(DEBOUNCING_t);
					}
					if (digitalRead(PB_PLUS) == 0)
					{
						displayBuffer[0] = 4;
						displayBuffer[1] = 2;
						writeDisplay();
						user_settings.timeFormat = H24;
						delay(DEBOUNCING_t);
					}
				}
				blink(_t.h, _t.m, 128);
				break;
			case 1:
				/* Set clock ColorMode */
				displayBuffer[0] = 9;
				displayBuffer[1] = 9;
				displayBuffer[2] = 9;
				displayBuffer[3] = 9;
				writeDisplay();
				// No color.
				colours[0] = rainbow[NO_COLOR];
				colours[1] = rainbow[NO_COLOR];
				colours[2] = rainbow[NO_COLOR];
				colours[3] = rainbow[NO_COLOR];
				led_strip_write(colours, DIGITS);
				// Save new led color mode.
				user_settings.colourMode = SOLIDCOLOR;
				while (1)
				{
					if (digitalRead(PB_PLUS) == 0)
					{
						// Solid color.
						rainbowMode(false);
						if (user_settings.colourMode == RAINBOW)
						{
							user_settings.colour = rgb;
						}
						if (user_settings.colour >= 359)
						{
							user_settings.colour = 0;
						}
						else
						{
							user_settings.colour++;
						}
						colours[0] = rainbow[user_settings.colour];
						colours[1] = rainbow[user_settings.colour];
						colours[2] = rainbow[user_settings.colour];
						colours[3] = rainbow[user_settings.colour];
						led_strip_write(colours, DIGITS);
						displayBuffer[0] = user_settings.colour % 10;
						displayBuffer[1] = (user_settings.colour / 10) % 10;
						displayBuffer[2] = (user_settings.colour / 100) % 10;
						displayBuffer[3] = NO_DIGIT;
						writeDisplay();
						//  Save new led color mode.
						user_settings.colourMode = SOLIDCOLOR;
						// Button debouncing.
						delay(DEBOUNCING_t);
					}
					if (digitalRead(PB_MENU) == 0)
					{
						// Restore menu display.
						rainbowMode(false);
						colours[0] = rainbow[NO_COLOR];
						colours[1] = rainbow[NO_COLOR];
						colours[2] = rainbow[NO_COLOR];
						colours[3] = rainbow[NO_COLOR];
						led_strip_write(colours, DIGITS);
						// Button debouncing.
						delay(DEBOUNCING_t);
						break;
					}
					if (digitalRead(PB_MINUS) == 0)
					{
						displayBuffer[0] = 9;
						displayBuffer[1] = 9;
						displayBuffer[2] = 9;
						displayBuffer[3] = 9;
						writeDisplay();
						if (user_settings.colourMode == RAINBOW)
						{
							// No color.
							rainbowMode(false);
							colours[0] = rainbow[NO_COLOR];
							colours[1] = rainbow[NO_COLOR];
							colours[2] = rainbow[NO_COLOR];
							colours[3] = rainbow[NO_COLOR];
							led_strip_write(colours, DIGITS);
							// Save new led color mode.
							user_settings.colourMode = SOLIDCOLOR;
						}
						else
						{
							// Rainbow mode
							rgb = user_settings.colour;
							rainbowMode(true);
							// Save new led color mode.
							user_settings.colourMode = RAINBOW;
						}
						// Button debouncing.
						delay(DEBOUNCING_t);
					}
				}
				break;
			case 2:
				/* Set Display on/off Time */
				while (1)
				{
					// Wake up time.
					displayBuffer[0] = 0;
					displayBuffer[1] = 0;
					displayBuffer[2] = 0;
					displayBuffer[3] = 0;
					writeDisplay();
					user_settings.wakeUpClk = getdata(23, UPPER);
					// Shut down time.
					displayBuffer[0] = 0;
					displayBuffer[1] = 0;
					displayBuffer[2] = 0;
					displayBuffer[3] = 0;
					writeDisplay();
					user_settings.gNightClk = getdata(23, UPPER);
					if (user_settings.wakeUpClk >= user_settings.gNightClk)
					{
						blink(user_settings.wakeUpClk, user_settings.gNightClk, 1);
					}
					else
					{
						blink(user_settings.wakeUpClk, user_settings.gNightClk, 128);
						break;
					}
				}
				break;
			case 3:
				/* Menu Quit */
				// Restore led colours.
				colours[0] = rainbow[user_settings.colour];
				colours[1] = rainbow[user_settings.colour];
				colours[2] = rainbow[user_settings.colour];
				colours[3] = rainbow[user_settings.colour];
				led_strip_write(colours, DIGITS);
				if (user_settings.colourMode == RAINBOW)
				{
					// Rainbow mode
					rgb = user_settings.colour;
					rainbowMode(true);
				}
				delay(DEBOUNCING_t);
				return;
			}
			menu_index = 3;
			displayBuffer[0] = menu_index;
			displayBuffer[1] = NO_DIGIT;
			displayBuffer[2] = NO_DIGIT;
			displayBuffer[3] = NO_DIGIT;
			writeDisplay();
			rainbowMode(false);
			colours[0] = rainbow[NO_COLOR];
			colours[1] = rainbow[NO_COLOR];
			colours[2] = rainbow[NO_COLOR];
			colours[3] = rainbow[NO_COLOR];
			led_strip_write(colours, DIGITS);
		}
	}
}

void rainbowMode(bool var)
{
	if (var)
	{
		TIMSK2 |= 0x02; // Enable interrupt on OCR2A
	}
	else
	{
		TIMSK2 &= ~(0x02); // Disable interrupt on OCR2A
	}
}

void slot(uint8_t hour)
{
	uint8_t tmp[4] = {0, 0, hour % 10, (hour / 10) % 10};

	for (uint8_t i = 0; i < 4; i++)
	{
		for (uint8_t j = 0; j < (40 - i * 10); j++)
		{
			for (uint8_t k = i; k < 4; k++)
			{
				if (displayBuffer[k] == 9)
				{
					displayBuffer[k] = 0;
				}
				else
				{
					displayBuffer[k] = displayBuffer[k] + 1;
				}
			}
			writeDisplay();
			delay(60);
		}

		for (uint8_t j = 0; j < 10; j++)
		{
			if (displayBuffer[i] == tmp[i])
			{
				break;
			}

			for (uint8_t k = i; k < 4; k++)
			{
				if (displayBuffer[k] == 9)
				{
					displayBuffer[k] = 0;
				}
				else
				{
					displayBuffer[k] = displayBuffer[k] + 1;
				}
			}
			writeDisplay();
			delay(60);
		}
	}
}

void blink(uint8_t _h, uint8_t _m, uint16_t _colour)
{
	displayBuffer[0] = _m % 10;
	displayBuffer[1] = (_m / 10) % 10;
	// Select 12-24 time format.
	if (user_settings.timeFormat)
	{
		displayBuffer[2] = _h % 10;
		displayBuffer[3] = (_h / 10) % 10;
	}
	else
	{
		if (_h >= 12)
		{
			_h -= 12;
		}
		displayBuffer[2] = _h % 10;
		displayBuffer[3] = (_h / 10) % 10;
	}
	writeDisplay();
	colours[0] = rainbow[NO_COLOR];
	colours[1] = rainbow[NO_COLOR];
	colours[2] = rainbow[NO_COLOR];
	colours[3] = rainbow[NO_COLOR];
	led_strip_write(colours, DIGITS);
	delay(300);

	colours[0] = rainbow[_colour];
	colours[1] = rainbow[_colour];
	colours[2] = rainbow[_colour];
	colours[3] = rainbow[_colour];
	led_strip_write(colours, DIGITS);
	delay(250);

	colours[0] = rainbow[NO_COLOR];
	colours[1] = rainbow[NO_COLOR];
	colours[2] = rainbow[NO_COLOR];
	colours[3] = rainbow[NO_COLOR];
	led_strip_write(colours, DIGITS);
	delay(250);

	colours[0] = rainbow[_colour];
	colours[1] = rainbow[_colour];
	colours[2] = rainbow[_colour];
	colours[3] = rainbow[_colour];
	led_strip_write(colours, DIGITS);
	delay(250);

	colours[0] = rainbow[NO_COLOR];
	colours[1] = rainbow[NO_COLOR];
	colours[2] = rainbow[NO_COLOR];
	colours[3] = rainbow[NO_COLOR];
	led_strip_write(colours, DIGITS);
	delay(250);
}

ISR(TIMER2_COMPA_vect)
{
	if (rgb > 358)
	{
		rgb = 0;
	}
	else
	{
		rgb++;
	}

	rgb_colour _cs[4] = {rainbow[rgb], rainbow[rgb], rainbow[rgb], rainbow[rgb]};
	led_strip_write(_cs, DIGITS);
}
