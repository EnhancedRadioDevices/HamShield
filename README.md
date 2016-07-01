# HamShield

The i2c-comms branch is not intended for use with a stock HamShield.

To use i2c communications with a HamShield 09 or above, you must first make several hardware changes to the board.

1. Remove R2
2. Short the pads of R17 (either with a wire or a low value resistor).

Then make sure that you pull the nCS pin correctly. Changing nCS changes the I2C address of the HamShield:

- nCS is logic high: A1846S_DEV_ADDR_SENHIGH 0b0101110
- nCS is logic low: A1846S_DEV_ADDR_SENLOW  0b1110001

HamShield Arduino Library and Example Sketches

This repository is meant to be checked out into your Arduino application's libraries folder. After reloading the application, the library and example sketches should be available for use.

For overview, help, tricks, tips, and more, check out the wiki: 

https://github.com/EnhancedRadioDevices/HamShield/wiki