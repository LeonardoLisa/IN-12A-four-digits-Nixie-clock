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

#include "SK6812.h"
#include "rainbow_table.h"

#define DIGITS static_cast<uint8_t>(4)
#define NO_DIGIT 10

#define CLK 4
#define DATA 5
#define LATCH 8
#define LED_DATA 13 // must be pin 13

#define REFRESH static_cast<unsigned long>(1000)


void writeDisplay();
void clockBegin();
void chkDisplay();
void chkLed();
void slot(uint8_t hour);

uint8_t displayBuffer[DIGITS];
rgb_colour colours[DIGITS];

void setup()
{
	clockBegin();

	chkLed();
	chkDisplay();
	
	for (uint8_t i = 0; i < 25; i++)
	{
		slot(i);
		delay(1000);
	}
}

void loop()
{
	for (uint16_t  i = 0; i < 10000; i++)
	{
		displayBuffer[0] = i % 10;
		displayBuffer[1] = (i / 10) % 10;
		displayBuffer[2] = (i / 100) % 10;
		displayBuffer[3] = (i / 1000) % 10;
		writeDisplay();

		delay(1000);
	}
}

void clockBegin()
{
	pinMode(CLK, OUTPUT);
	pinMode(DATA, OUTPUT);
	pinMode(LATCH, OUTPUT);
	digitalWrite(CLK, LOW);
	digitalWrite(DATA, HIGH);
	digitalWrite(LATCH, LOW);

	pinMode(LED_DATA, OUTPUT);
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

void slot(uint16_t hour)
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
