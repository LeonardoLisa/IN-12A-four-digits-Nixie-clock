/*
 *	ds3231.h
 *
 *	Created: 09/03/2021 10:09:18
 *	Author: Leonardo Lisa
 *
 */

#include <Wire.h>

#ifndef __DS3231_H__
#define __DS3231_H__

#define SLA_ADDRESS 0x68

#define FORCE_TMP_UPDATE
// If not defined temperature will be updated every 65s.


struct Date {
	uint8_t s;
	uint8_t m;
	uint8_t h;
	uint8_t day;
	uint8_t month;
	uint16_t year;
} ;

struct Time {
	uint8_t s;
	uint8_t m;
	uint8_t h;
} ;

// Initialize ds3231
void ds3231_init(void) {
	Wire.begin(); // join i2c bus (address optional for master)

	Wire.beginTransmission(SLA_ADDRESS);
	Wire.write(0x0E); // Sets register pointer.
 // Address 0x0E
	Wire.write(0x04);
	// Bit 2 enable square wave is output on the INT/SQW pin.
	// Bits 3-4 select 1Hz frequency.
  // Address 0x0F
	Wire.write(0x00);
	// Bit 3 enable 32kHz Output (EN32kHz).
	
	Wire.endTransmission(true);
}

// Get ds3231 temp from internal sensor.
uint16_t ds3231_temp(void) {
  uint8_t rawdata;
	uint16_t temp = 0;

#ifdef FORCE_TMP_UPDATE
  // Check (BSY) bit. This bit indicates the device is busy.
  Wire.beginTransmission(SLA_ADDRESS);
  Wire.write(0x0F); // Sets register pointer.
  Wire.endTransmission();
  Wire.requestFrom(SLA_ADDRESS, 1, true);
  // Address 0x0F.
  rawdata = Wire.read();
  if((rawdata & 0x04) == 0) {
    Wire.beginTransmission(SLA_ADDRESS);
    Wire.write(0x0E); // Sets register pointer.
    // Address 0x0E
    Wire.write(0x24);
    // Bit 2 enable square wave is output on the INT/SQW pin.
    // Bits 3-4 select 1Hz frequency.
    // Bit 5 forces the temperature sensor to convert the temperature into digital code.
    Wire.endTransmission(true);

    // Waits for temperature update.
    Wire.beginTransmission(SLA_ADDRESS);
    Wire.write(0x0F); // Sets register pointer.
    Wire.endTransmission();
    Wire.requestFrom(SLA_ADDRESS, 1, true);
    // Address 0x0F.
    rawdata = Wire.read();
    while((rawdata & 0x04) == 0x04) {
      Wire.beginTransmission(SLA_ADDRESS);
      Wire.write(0x0F); // Sets register pointer.
      Wire.endTransmission();
      Wire.requestFrom(SLA_ADDRESS, 1, true);
      // Address 0x0F.
      rawdata = Wire.read();
    }
  }
#endif

	// Master Transmitter Mode.
	Wire.beginTransmission(SLA_ADDRESS);
	Wire.write(0x11); // Sets register pointer.
	Wire.endTransmission();
	Wire.requestFrom(SLA_ADDRESS, 2, true);

	// Address 0x11.
  rawdata = Wire.read();
	temp = (static_cast<uint16_t>(rawdata) * 100);
	// Address 0x12.
  rawdata = Wire.read();
	temp += (rawdata >> 6) * 25;
	
	return temp;
}

// Get current time/date.
void ds3231_get_Time(Time *_T) {
	uint8_t rawdata;

	// Master Transmitter Mode.
	Wire.beginTransmission(SLA_ADDRESS);
	Wire.write(0x00); // Sets register pointer.
	Wire.endTransmission();
	Wire.requestFrom(SLA_ADDRESS, 3, true);

	rawdata = Wire.read(); // Address 0x00.
	_T->s = (rawdata >> 4) * 10 + (rawdata & 0x0f);
	
	rawdata = Wire.read(); // Address 0x01.
	_T->m = (rawdata >> 4) * 10 + (rawdata & 0x0f);
	
	rawdata = Wire.read(); // Address 0x02.
	_T->h = ((rawdata & 0x30) >> 4) * 10 + (rawdata & 0x0f);
}

void ds3231_get_Time(Date *_T) {
	uint8_t rawdata;

	// Master Transmitter Mode.
	Wire.beginTransmission(SLA_ADDRESS);
	Wire.write(0x00); // Sets register pointer.
	Wire.endTransmission();
	Wire.requestFrom(SLA_ADDRESS, 7, true);

	rawdata = Wire.read(); // Address 0x00.
	_T->s = (rawdata >> 4) * 10 + (rawdata & 0x0f);
	
	rawdata = Wire.read(); // Address 0x01.
	_T->m = (rawdata >> 4) * 10 + (rawdata & 0x0f);
	
	rawdata = Wire.read(); // Address 0x02.
	_T->h = ((rawdata & 0x30) >> 4) * 10 + (rawdata & 0x0f);
	
	rawdata = Wire.read(); // Address 0x03.
	rawdata = Wire.read(); // Address 0x04.
	_T->day = (rawdata >> 4) * 10 + (rawdata & 0x0f);
	
	rawdata = Wire.read(); // Address 0x05.
	_T->month = (!!(rawdata & 0x10)) * 10 + (rawdata & 0x0f);
	_T->year = 2000 + static_cast<uint16_t>(!!(rawdata & 0x80)) * 100;
	
	rawdata = Wire.read(); // Address 0x06.
	_T->year += (rawdata >> 4) * 10 + (rawdata & 0x0f);
}

void ds3231_update(Time *_T) {
	// Master Transmitter Mode.
	Wire.beginTransmission(SLA_ADDRESS);
	Wire.write(0x00); // Sets register pointer.

	// Address 0x00
	Wire.write(((_T->s / 10) << 4) + (_T->s % 10));
	// Address 0x01
	Wire.write(((_T->m / 10) << 4) + (_T->m % 10));
	// Address 0x02
	Wire.write(((_T->h / 10) << 4) + (_T->h % 10));
	
	Wire.endTransmission(true);
}

void ds3231_update(Date *_T) {
	Time __T;
	__T.s = _T->s;
	__T.m = _T->m;
	__T.h = _T->h;
	ds3231_update(&__T);

	// Master Transmitter Mode.
	Wire.beginTransmission(SLA_ADDRESS);
	Wire.write(0x04); // Sets register pointer.

	// Address 0x04.
	Wire.write(((_T->day / 10) << 4) + (_T->day % 10));
	// Address 0x05.
	if (_T->year >= 2100)
	{
		Wire.write(((_T->month / 10) << 4) + (_T->month % 10) + 0b10000000);
	}
	else
	{
		Wire.write(((_T->month / 10) << 4) + (_T->month % 10));
	}
	// Address 0x06.
	if (_T->year >= 2100)
	{
		Wire.write((((_T->year - 2100) / 10) << 4) + ((_T->year - 2100) % 10));
	}
	else
	{
		Wire.write((((_T->year - 2000) / 10) << 4) + ((_T->year - 2000) % 10));
	}

	Wire.endTransmission(true);
}

#endif /* DS3231_H_ */
