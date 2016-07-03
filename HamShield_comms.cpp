/*
 * Based loosely on I2Cdev by Jeff Rowberg
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

    int8_t count = 0;
    uint32_t t1 = millis();
	uint8_t timeout = 1000;

	Wire.beginTransmission(devAddr);
    Wire.write(regAddr);
    Wire.endTransmission(false);
    Wire.requestFrom((int)devAddr, 2); // length=words, this wants bytes
        
    bool msb = true; // starts with MSB, then LSB
    for (; Wire.available() && count < 1 && (timeout == 0 || millis() - t1 < timeout);) {
        if (msb) {
            // first byte is bits 15-8 (MSb=15)
            data[0] = Wire.read() << 8;
        } else {
            // second byte is bits 7-0 (LSb=0)
            data[0] |= Wire.read();
            count++;
        }
        msb = !msb;
    }
        
    Wire.endTransmission();

    if (timeout > 0 && millis() - t1 >= timeout && count < 1) count = -1; // timeout
    
    return count;

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
    uint8_t status = 0;

	Wire.beginTransmission(devAddr);
    Wire.write(regAddr); // send address

    Wire.write((uint8_t)(data >> 8));    // send MSB
    Wire.write((uint8_t)data);         // send LSB
  
    status = Wire.endTransmission();
    return status == 0;
}