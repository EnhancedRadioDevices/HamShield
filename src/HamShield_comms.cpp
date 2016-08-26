/*
 * Based loosely on I2Cdev by Jeff Rowberg, except for all kludgy bit-banging
 */

#include "HamShield_comms.h"

int8_t HSreadBitW(uint8_t devAddr, uint8_t regAddr, uint8_t bitNum, uint16_t *data)
{
    uint16_t b;
    uint8_t count = HSreadWord(devAddr, regAddr, &b);
    *data = b & (1 << bitNum);
    return count;
}

int8_t HSreadBitsW(uint8_t devAddr, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint16_t *data)
{
	uint8_t count;
    uint16_t w;
    if ((count = HSreadWord(devAddr, regAddr, &w)) != 0) {
        uint16_t mask = ((1 << length) - 1) << (bitStart - length + 1);
        w &= mask;
        w >>= (bitStart - length + 1);
        *data = w;
    }
    return count;
}

int8_t HSreadWord(uint8_t devAddr, uint8_t regAddr, uint16_t *data)
{
	//return I2Cdev::readWord(devAddr, regAddr, data);
	
	uint8_t temp;
	uint16_t temp_dat;
	// bitbang for great justice!
	*data = 0;
	pinMode(DAT, OUTPUT);
	regAddr = regAddr | (1 << 7);
	
	digitalWrite(devAddr, 0); //PORTC &= ~(1<<1); //devAddr used as chip select
	for (int i = 0; i < 8; i++) {
		temp = ((regAddr & (0x80 >> i)) != 0);
		digitalWrite(CLK, 0); //PORTC &= ~(1<<5); //
		digitalWrite(DAT, temp);
		digitalWrite(CLK, 1); //PORTC |= (1<<5); //
	}
	// change direction of DAT
	pinMode(DAT, INPUT); //	DDRC &= ~(1<<4); //
	for (int i = 15; i >= 0; i--) {
		digitalWrite(CLK, 0); //PORTC &= ~(1<<5); //
		digitalWrite(CLK, 1); //PORTC |= (1<<5); //
		temp_dat = digitalRead(DAT); //((PINC & (1<<4)) != 0);
		temp_dat = temp_dat << i;
		*data |= temp_dat;
	}
	digitalWrite(devAddr, 1); //PORTC |= (1<<1);// CS
	
	return 1;
}


bool HSwriteBitW(uint8_t devAddr, uint8_t regAddr, uint8_t bitNum, uint16_t data)
{
	uint16_t w;
    HSreadWord(devAddr, regAddr, &w);
    w = (data != 0) ? (w | (1 << bitNum)) : (w & ~(1 << bitNum));
    return HSwriteWord(devAddr, regAddr, w);
}

bool HSwriteBitsW(uint8_t devAddr, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint16_t data)
{
uint16_t w;
    if (HSreadWord(devAddr, regAddr, &w) != 0) {
        uint16_t mask = ((1 << length) - 1) << (bitStart - length + 1);
        data <<= (bitStart - length + 1); // shift data into correct position
        data &= mask; // zero all non-important bits in data
        w &= ~(mask); // zero all important bits in existing word
        w |= data; // combine data with existing word
        return HSwriteWord(devAddr, regAddr, w);
    } else {
        return false;
    }
}

bool HSwriteWord(uint8_t devAddr, uint8_t regAddr, uint16_t data)
{
	//return I2Cdev::writeWord(devAddr, regAddr, data);
	
	uint8_t temp_reg;
	uint16_t temp_dat;
	
	//digitalWrite(13, HIGH);
	
	// bitbang for great justice!
	pinMode(DAT, OUTPUT);
	regAddr = regAddr & ~(1 << 7);
	
	digitalWrite(devAddr, 0); // PORTC &= ~(1<<1); //CS
	for (int i = 0; i < 8; i++) {
		temp_reg = ((regAddr & (0x80 >> i)) != 0);
		digitalWrite(CLK, 0); //PORTC &= ~(1<<5); //
		digitalWrite(DAT, regAddr & (0x80 >> i));
		digitalWrite(CLK, 1); // PORTC |= (1<<5); //
	}
	for (int i = 0; i < 16; i++) {
		temp_dat = ((data & (0x8000 >> i)) != 0);
		digitalWrite(CLK, 0); //PORTC &= ~(1<<5); //
		digitalWrite(DAT, temp_dat);
		digitalWrite(CLK, 1); // PORTC |= (1<<5); //
	}
	
	digitalWrite(devAddr, 1); //PORTC |= (1<<1); //CS
	
	return true;
}