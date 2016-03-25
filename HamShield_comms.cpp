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
	cli();
	DDRC |= ((1<<5) | (1<<4)); // set direction to output
	sei();
	regAddr = regAddr | (1 << 7);
	
	cli();
	PORTC &= ~(1<<1); //digitalWrite(nSEN, 0);
	sei();
	for (int i = 0; i < 8; i++) {
		temp = ((regAddr & (0x80 >> i)) != 0);
		cli();
		PORTC &= ~(1<<5); //digitalWrite(CLK, 0);
		sei();
		//digitalWrite(DAT, regAddr & (0x80 >> i));
		temp = (PORTC & ~(1<<4)) + (temp << 4);
		cli();
		PORTC = temp;
		sei();
		delayMicroseconds(9);
		cli();
		PORTC |= (1<<5); //digitalWrite(CLK, 1);
		sei();
		delayMicroseconds(9);
	}
	// change direction of DAT
	cli();
	DDRC &= ~(1<<4); //pinMode(DAT, INPUT);
	sei();
	for (int i = 15; i >= 0; i--) {
		cli();
		PORTC &= ~(1<<5); //digitalWrite(CLK, 0);
		sei();
		delayMicroseconds(9);
		cli();
		PORTC |= (1<<5); //digitalWrite(CLK, 1);
		sei();
		cli();
		temp_dat = ((PINC & (1<<4)) != 0);
		sei();
		temp_dat = temp_dat << i;
		*data |= temp_dat; // digitalRead(DAT);
		delayMicroseconds(9);
	}
	cli();
	PORTC |= (1<<1);//digitalWrite(nSEN, 1);
	
	DDRC &= ~((1<<5) | (1<<4)); // set direction all input (for ADC)
	sei();
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
	cli();
	DDRC |= ((1<<5) | (1<<4)); // set direction all output
	//PORTC |= (1<<5) & (1<<4);
	sei();
	regAddr = regAddr & ~(1 << 7);
	
	cli();
	PORTC &= ~(1<<1); //digitalWrite(nSEN, 0);
	sei();
	for (int i = 0; i < 8; i++) {
		temp_reg = ((regAddr & (0x80 >> i)) != 0);
		cli();
		PORTC &= ~(1<<5); //digitalWrite(CLK, 0);
		sei();
		//digitalWrite(DAT, regAddr & (0x80 >> i));
		temp_reg = (PORTC & ~(1<<4)) + (temp_reg << 4);
		cli();
		PORTC = temp_reg;
		sei();
		delayMicroseconds(8);
		cli();
		PORTC |= (1<<5); //digitalWrite(CLK, 1);
		sei();
		delayMicroseconds(10);
	}
	for (int i = 0; i < 16; i++) {
		temp_dat = ((data & (0x8000 >> i)) != 0);
		cli();
		PORTC &= ~(1<<5); //digitalWrite(CLK, 0);
		sei();
		//digitalWrite(DAT, data & (0x80000 >> i));
		temp_reg = (PORTC & ~(1<<4)) + (temp_dat << 4);
		cli();
		PORTC = temp_reg;
		sei();
		delayMicroseconds(7);
		cli();
		PORTC |= (1<<5); //digitalWrite(CLK, 1);
		sei();
		delayMicroseconds(10);
	}
	cli();
	PORTC |= (1<<1); //digitalWrite(nSEN, 1);
	
	DDRC &= ~((1<<5) | (1<<4)); // set direction to input for ADC
	sei();
	return true;
}