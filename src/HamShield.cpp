// HamShield library collection
// Based on Programming Manual rev. 2.0, 5/19/2011 (RM-MPU-6000A-00)
// 11/22/2013 by Morgan Redfield <redfieldm@gmail.com>
// 04/26/2015 various changes Casey Halverson <spaceneedle@gmail.com>

#include "HamShield.h"
#include "stdint.h"
#include "math.h"

#if defined(__AVR__)
#include <avr/pgmspace.h>
#define MORSE_TABLE_PROGMEM
#else
    // get rid of progmem for now and just put these tables in flash/program space
   #define PROGMEM
#endif


/* don't change this regulatory value, use dangerMode() and safeMode() instead */
bool restrictions = true; 
uint16_t old_dtmf_reg;

/* channel lookup tables */

const uint32_t FRS[] PROGMEM = {0,462562,462587,462612,462637,462662,462687,462712,467562,467587,467612,467637,467662,467687,467712};

const uint32_t GMRS[] PROGMEM = {0,462550,462575,462600,462625,462650,462675,462700,462725};

const uint32_t MURS[] PROGMEM = {0,151820,151880,151940,154570,154600};

const uint32_t WX[] PROGMEM = {0,162550,162400,162475,162425,162450,162500,162525};


unsigned int morse_freq = 600;
unsigned int morse_dot_millis = 100;

/* morse code lookup table */
// This is the Morse table in reverse binary format.
// It will occupy 108 bytes of memory (or program memory if defined)

#define MORSE_TABLE_LENGTH 54
#ifndef MORSE_TABLE_PROGMEM
const struct asciiMorse {
    char ascii;
    uint8_t itu;
} asciiMorse[MORSE_TABLE_LENGTH] = {
    { 'E', 0b00000010 }, // .
    { 'T', 0b00000011 }, // -
    { 'I', 0b00000100 }, // ..
    { 'N', 0b00000101 }, // -.
    { 'A', 0b00000110 }, // .-
    { 'M', 0b00000111 }, // --
    { 'S', 0b00001000 }, // ...
    { 'D', 0b00001001 }, // -..
    { 'R', 0b00001010 }, // .-.
    { 'G', 0b00001011 }, // --.
    { 'U', 0b00001100 }, // ..-
    { 'K', 0b00001101 }, // -.-
    { 'W', 0b00001110 }, // .--
    { 'O', 0b00001111 }, // ---
    { 'H', 0b00010000 }, // ....
    { 'B', 0b00010001 }, // -...
    { 'L', 0b00010010 }, // .-..
    { 'Z', 0b00010011 }, // --..
    { 'F', 0b00010100 }, // ..-.
    { 'C', 0b00010101 }, // -.-.
    { 'P', 0b00010110 }, // .--.
    { 'V', 0b00011000 }, // ...-
    { 'X', 0b00011001 }, // -..-
    { 'Q', 0b00011011 }, // --.-
    { 'Y', 0b00011101 }, // -.--
    { 'J', 0b00011110 }, // .---
    { '5', 0b00100000 }, // .....
    { '6', 0b00100001 }, // -....
    { '&', 0b00100010 }, // .-...
    { '7', 0b00100011 }, // --...
    { '8', 0b00100111 }, // ---..
    { '/', 0b00101001 }, // -..-.
    { '+', 0b00101010 }, // .-.-.
    { '(', 0b00101101 }, // -.--.
    { '9', 0b00101111 }, // ----.
    { '4', 0b00110000 }, // ....-
    { '=', 0b00110001 }, // -...-
    { '3', 0b00111000 }, // ...--
    { '2', 0b00111100 }, // ..---
    { '1', 0b00111110 }, // .----
    { '0', 0b00111111 }, // -----
    { ':', 0b01000111 }, // ---...
    { '?', 0b01001100 }, // ..--..
    { '"', 0b01010010 }, // .-..-.
    { ';', 0b01010101 }, // -.-.-.
    { '@', 0b01010110 }, // .--.-.
    { '\047', 0b01011110 }, // (') .----.
    { '-', 0b01100001 }, // -....-
    { '.', 0b01101010 }, // .-.-.-
    { '_', 0b01101100 }, // ..--.-
    { ')', 0b01101101 }, // -.--.-
    { ',', 0b01110011 }, // --..--
    { '!', 0b01110101 }, // -.-.--
    { '$', 0b11001000 } // ...-..-
};
#else
// This is a program memory variant, using 16 bit words for storage instead.
const uint16_t asciiMorseProgmem[] PROGMEM = {
  0x4502, 0x5403, 0x4904, 0x4E05, 0x4106, 0x4D07, 0x5308, 0x4409, 0x520A,
  0x470B, 0x550C, 0x4B0D, 0x570E, 0x4F0F, 0x4810, 0x4211, 0x4C12, 0x5A13,
  0x4614, 0x4315, 0x5016, 0x5618, 0x5819, 0x511B, 0x591D, 0x4A1E, 0x3520,
  0x3621, 0x2622, 0x3723, 0x3827, 0x2F29, 0x2B2A, 0x282D, 0x392F, 0x3430,
  0x3D31, 0x3338, 0x323C, 0x313E, 0x303F, 0x3A47, 0x3F4C, 0x2252, 0x3B55,
  0x4056, 0x275E, 0x2D61, 0x2E6A, 0x5F6C, 0x296D, 0x2C73, 0x2175, 0x24C8
};
#endif // MORSE_TABLE_PROGMEM

/* 2200 Hz -- This lookup table should be deprecated */

const unsigned char AFSK_mark[] PROGMEM = { 154, 249, 91, 11, 205, 216, 25, 68, 251, 146, 0, 147, 250, 68, 24, 218, 203, 13, 88, 254, 128, 1, 167, 242, 52, 37, 231, 186, 5, 108, 255, 108, 5, 186, 231, 37, 52, 242, 167, 1, 128, 254, 88, 13, 203, 218, 24, 69, 250, 147, 0, 147, 250, 69, 24, 218, 203, 13, 88, 255, 127, 2, 165, 245, 48 };

/* 1200 Hz -- This lookup table should be deprecated */

const unsigned char AFSK_space[] PROGMEM = { 140, 228, 250, 166, 53, 0, 53, 166, 249, 230, 128, 24, 7, 88, 203, 255, 203, 88, 7, 24, 128, 230, 249, 167, 53, 0, 53, 167, 249, 230, 128, 24, 6, 88, 202, 255, 202, 88, 6, 24, 127, 231, 249, 167, 52, 0, 52, 167, 248, 231, 127, 25, 6, 89, 202, 255, 202, 89, 6, 25, 127, 231, 248, 167, 53, 0, 54, 165, 251, 227, 133, 14}; 


/** Specific address constructor.
 * @param chip select pin for HamShield
 * @see A1846S_DEFAULT_ADDRESS
 * @see A1846S_ADDRESS_AD0_LOW
 * @see A1846S_ADDRESS_AD0_HIGH
 */
HamShield::HamShield(uint8_t ncs_pin, uint8_t clk_pin, uint8_t dat_pin, uint8_t mic_pin) {
    devAddr = ncs_pin;
    hs_mic_pin = mic_pin;
    
    HSsetPins(ncs_pin, clk_pin, dat_pin);
}


/** Power on and prepare for general usage.
 * 
 */
void HamShield::initialize() {  
    initialize(true);
}

/** Power on and prepare for general usage.
 * 
 */
void HamShield::initialize(bool narrowBand) {  
   // Note: these initial settings are for UHF 12.5kHz channel
   // see the A1846S register table and initial settings for more info
   
    uint16_t tx_data;
  
    // reset all registers in A1846S
    softReset();
    
    //set up clock to ues 12-14MHz
    setClkMode(1);
    
    // set up GPIO voltage (want 3.3V)
    tx_data = 0x03AC; // default is 0x32C
    HSwriteWord(devAddr, 0x09, tx_data);

    tx_data = 0x47E0; //0x43A0; // 0x7C20; //
    HSwriteWord(devAddr, 0x0A, tx_data); // pga gain [10:6]
    tx_data = 0xA100;
    HSwriteWord(devAddr, 0x13, tx_data);
    tx_data = 0x5001;
    HSwriteWord(devAddr, 0x1F, tx_data); // GPIO7->VOX, GPIO0->CTC/DCS
    
    
    tx_data = 0x0031;
    HSwriteWord(devAddr, 0x31, tx_data);
    tx_data = 0x0AF2; //
    HSwriteWord(devAddr, 0x33, tx_data); // agc number
    
    tx_data = 0x067F; //0x0601; //0x470F;
    HSwriteWord(devAddr, 0x41, tx_data); // voice gain tx [6:0]
    tx_data = 0x02FF; // using 0x04FF to avoid tx voice delay
    HSwriteWord(devAddr, 0x44, tx_data); // tx gain [11:8]
    tx_data = 0x7F2F;
    HSwriteWord(devAddr, 0x47, tx_data);
    tx_data = 0x2C62;
    HSwriteWord(devAddr, 0x4F, tx_data);
    tx_data = 0x0094;
    HSwriteWord(devAddr, 0x53, tx_data); // compressor update time (bits 6:0, 5.12ms per unit)
    tx_data = 0x2A18;
    HSwriteWord(devAddr, 0x54, tx_data);
    tx_data = 0x0081;
    HSwriteWord(devAddr, 0x55, tx_data);
    tx_data = 0x0B22;
    HSwriteWord(devAddr, 0x56, tx_data); // sq detect time
    tx_data = 0x1C00;
    HSwriteWord(devAddr, 0x57, tx_data);
    tx_data = 0x800D;
    HSwriteWord(devAddr, 0x58, tx_data); 
    tx_data = 0x0EDB;
    HSwriteWord(devAddr, 0x5A, tx_data); // sq and noise detect times
    tx_data = 0x3FFF;
    HSwriteWord(devAddr, 0x63, tx_data); // pre-emphasis bypass

    // calibration
    tx_data = 0x00A4;
    HSwriteWord(devAddr, 0x30, tx_data);
    HSdelay(100);
    tx_data = 0x00A6;
    HSwriteWord(devAddr, 0x30, tx_data);
    HSdelay(100);
    tx_data = 0x0006;
    HSwriteWord(devAddr, 0x30, tx_data);
    HSdelay(100);


    // set band width
    if (narrowBand) {
        setupNarrowBand();
    } else {
        setupWideBand();
    }
    
    HSdelay(100);
    
    /*
    // setup default values
    frequency(446000);
    //setVolume1(0xF);
    //setVolume2(0xF);
    setModeReceive();
    setTxSourceMic();
    setRfPower(0);
    setSQLoThresh(-80);
    setSQOn();
    */
    setDTMFIdleTime(50);
    setDTMFTxTime(60);
    setDTMFDetectTime(24);
    
    HSreadWord(devAddr, A1846S_DTMF_ENABLE_REG, radio_i2c_buf);
    old_dtmf_reg = radio_i2c_buf[0];
}


/** Set up the AU1846 in Narrow Band mode (12.5kHz).
 */
void HamShield::setupNarrowBand() {
    uint16_t tx_data;
    // setup for 12.5kHz channel width
    tx_data = 0x3D37;
    HSwriteWord(devAddr, 0x11, tx_data);    
    tx_data = 0x0100;
    HSwriteWord(devAddr, 0x12, tx_data);    
    tx_data = 0x1100;
    HSwriteWord(devAddr, 0x15, tx_data);
    tx_data = 0x4495;
    HSwriteWord(devAddr, 0x32, tx_data); // agc target power [11:6]
    tx_data = 0x2B8E;
    HSwriteWord(devAddr, 0x34, tx_data);
    tx_data = 0x40C3;
    HSwriteWord(devAddr, 0x3A, tx_data); // modu_det_sel sq setting
    tx_data = 0x0F1E;
    HSwriteWord(devAddr, 0x3C, tx_data); // pk_det_th sq setting [8:7]
    tx_data = 0x28D0;
    HSwriteWord(devAddr, 0x3F, tx_data); // rssi3_th sq setting
    tx_data = 0x20BE;
    HSwriteWord(devAddr, 0x48, tx_data);
    tx_data = 0x1BB7;
    HSwriteWord(devAddr, 0x60, tx_data);
    tx_data = 0x0A10; // use 0x1425 if there's an LNA
    HSwriteWord(devAddr, 0x62, tx_data);
    tx_data = 0x2494;
    HSwriteWord(devAddr, 0x65, tx_data);
    tx_data = 0xEB2E;
    HSwriteWord(devAddr, 0x66, tx_data);

        // AGC table
    tx_data = 0x0001;
    HSwriteWord(devAddr, 0x7F, tx_data);
    tx_data = 0x000C;
    HSwriteWord(devAddr, 0x05, tx_data);
    tx_data = 0x020C;
    HSwriteWord(devAddr, 0x06, tx_data);
    tx_data = 0x030C;
    HSwriteWord(devAddr, 0x07, tx_data);
    tx_data = 0x0324;
    HSwriteWord(devAddr, 0x08, tx_data);
    tx_data = 0x1344;
    HSwriteWord(devAddr, 0x09, tx_data);
    tx_data = 0x3F44;
    HSwriteWord(devAddr, 0x0A, tx_data);
    tx_data = 0x3F44;
    HSwriteWord(devAddr, 0x0B, tx_data);
    tx_data = 0x3F44;
    HSwriteWord(devAddr, 0x0C, tx_data);
    tx_data = 0x3F44;
    HSwriteWord(devAddr, 0x0D, tx_data);
    tx_data = 0x3F44;
    HSwriteWord(devAddr, 0x0E, tx_data);
    tx_data = 0x3F44;
    HSwriteWord(devAddr, 0x0F, tx_data);
    tx_data = 0xE0ED;
    HSwriteWord(devAddr, 0x12, tx_data);
    tx_data = 0xF2FE;
    HSwriteWord(devAddr, 0x13, tx_data);
    tx_data = 0x0A16;
    HSwriteWord(devAddr, 0x14, tx_data);
    tx_data = 0x2424;
    HSwriteWord(devAddr, 0x15, tx_data);
    tx_data = 0x2424;
    HSwriteWord(devAddr, 0x16, tx_data);
    tx_data = 0x2424;
    HSwriteWord(devAddr, 0x17, tx_data);
    tx_data = 0x0000;
    HSwriteWord(devAddr, 0x7F, tx_data);
    // end AGC table

}

/** Set up the AU1846 in Wide Band mode (25kHz).
 */
void HamShield::setupWideBand() {
    uint16_t tx_data;
    // setup for 25kHz channel width
    tx_data = 0x3D37;
    HSwriteWord(devAddr, 0x11, tx_data);    
    tx_data = 0x0100;
    HSwriteWord(devAddr, 0x12, tx_data);    
    tx_data = 0x1F00;
    HSwriteWord(devAddr, 0x15, tx_data);
    tx_data = 0x7564;
    HSwriteWord(devAddr, 0x32, tx_data); // agc target power [11:6]
    tx_data = 0x2B8E;
    HSwriteWord(devAddr, 0x34, tx_data);
    tx_data = 0x44C3;
    HSwriteWord(devAddr, 0x3A, tx_data); // modu_det_sel sq setting
    tx_data = 0x1930;
    HSwriteWord(devAddr, 0x3C, tx_data); // pk_det_th sq setting [8:7]
    tx_data = 0x29D2;
    HSwriteWord(devAddr, 0x3F, tx_data); // rssi3_th sq setting
    tx_data = 0x21C0;
    HSwriteWord(devAddr, 0x48, tx_data);
    tx_data = 0x101E;
    HSwriteWord(devAddr, 0x60, tx_data);
    tx_data = 0x3767; // use 0x1425 if there's an LNA
    HSwriteWord(devAddr, 0x62, tx_data);
    tx_data = 0x248A;
    HSwriteWord(devAddr, 0x65, tx_data);
    tx_data = 0xFFAE;
    HSwriteWord(devAddr, 0x66, tx_data);    

        // AGC table
    tx_data = 0x0001;
    HSwriteWord(devAddr, 0x7F, tx_data);
    tx_data = 0x000C;
    HSwriteWord(devAddr, 0x05, tx_data);
    tx_data = 0x0024;
    HSwriteWord(devAddr, 0x06, tx_data);
    tx_data = 0x0214;
    HSwriteWord(devAddr, 0x07, tx_data);
    tx_data = 0x0224;
    HSwriteWord(devAddr, 0x08, tx_data);
    tx_data = 0x0314;
    HSwriteWord(devAddr, 0x09, tx_data);
    tx_data = 0x0324;
    HSwriteWord(devAddr, 0x0A, tx_data);
    tx_data = 0x0344;
    HSwriteWord(devAddr, 0x0B, tx_data);
    tx_data = 0x0384;
    HSwriteWord(devAddr, 0x0C, tx_data);
    tx_data = 0x1384;
    HSwriteWord(devAddr, 0x0D, tx_data);
    tx_data = 0x1B84;
    HSwriteWord(devAddr, 0x0E, tx_data);
    tx_data = 0x3F84;
    HSwriteWord(devAddr, 0x0F, tx_data);
    tx_data = 0xE0EB;
    HSwriteWord(devAddr, 0x12, tx_data);
    tx_data = 0xF2FE;
    HSwriteWord(devAddr, 0x13, tx_data);
    tx_data = 0x0A16;
    HSwriteWord(devAddr, 0x14, tx_data);
    tx_data = 0x2424;
    HSwriteWord(devAddr, 0x15, tx_data);
    tx_data = 0x2424;
    HSwriteWord(devAddr, 0x16, tx_data);
    tx_data = 0x2424;
    HSwriteWord(devAddr, 0x17, tx_data);
    tx_data = 0x0000;
    HSwriteWord(devAddr, 0x7F, tx_data);
    // end AGC table
}

/** Verify the I2C connection.
 * Make sure the device is connected and responds as expected.
 * @return True if connection is valid, false otherwise
 */
bool HamShield::testConnection() {
    HSreadWord(devAddr, 0x00, radio_i2c_buf);
    return radio_i2c_buf[0] == 0x1846;
}


/** A1846S each register write is 24-bit long, including a 
 * r/nw bit, 7-bit register address , and 16-bit data (MSB 
 * is the first bit).
 *   R/W, A[6:0], D[15:0]
 *
 * Note (this shouldn't be necessary, since all ctl registers are below 0x7F)
 * If register address is more than 7FH, first write 0x0001 
 * to 7FH, and then write value to the address subtracted by 
 * 80H. Finally write 0x0000 to 7FH 
 * Example: writing 85H register address is 0x001F .
 *   Move 7FH 0x0001{}
 *   Move 05H 0x001F{} 05H=85H-80H
 *   Move 7FH 0x0000{}
 */

uint16_t HamShield::readCtlReg() {
  HSreadWord(devAddr, A1846S_CTL_REG, radio_i2c_buf);
  return radio_i2c_buf[0];
}

void HamShield::softReset() {
   uint16_t tx_data = 0x1;
   HSwriteWord(devAddr, A1846S_CTL_REG, tx_data);
   HSdelay(100); // Note: see A1846S setup info for timing guidelines
   tx_data = 0x4;
   HSwriteWord(devAddr, A1846S_CTL_REG, tx_data);
}

 
void HamShield::setFrequency(uint32_t freq_khz) {
    radio_frequency = (float) freq_khz;
    uint32_t freq_raw = freq_khz << 4; // shift by 4 to multiply by 16 (was shift by 3 in old 1846 chip)

    // turn off tx/rx
    HSwriteBitsW(devAddr, A1846S_CTL_REG, 6, 2, 0);
    
    // if we're using a 12MHz crystal and the frequency is
    // 136.5M,409.5M and 455M, then we have to do special stuff
    if (radio_frequency == 136500 ||
        radio_frequency == 490500 ||
        radio_frequency == 455000) {
        
        // set up AU1846 for funky freq
        HSwriteWord(devAddr, 0x05, 0x86D3);

    } else {
        // set up AU1846 for normal freq
        HSwriteWord(devAddr, 0x05, 0x8763);
    }
    
    // send top 16 bits to A1846S_FREQ_HI_REG    
    uint16_t freq_half = (uint16_t) (0x3FFF & (freq_raw >> 16));
    HSwriteWord(devAddr, A1846S_FREQ_HI_REG, freq_half);
    //  send bottom 16 bits to A1846S_FREQ_LO_REG
    freq_half = (uint16_t) (freq_raw & 0xFFFF);
    HSwriteWord(devAddr, A1846S_FREQ_LO_REG, freq_half);
    
       if (rx_active) {
        setRX(true);
    } else if (tx_active) {
        setTX(true);
    }
}

uint32_t HamShield::getFrequency() {
  return (uint32_t) radio_frequency;
}

float HamShield::getFrequency_float() {
  return radio_frequency;
}

void HamShield::setTxBand2m() {
  setGpioLow(4); // V1
  setGpioHi(5); // V2
}

void HamShield::setTxBand1_2m() {
  setGpioHi(4); // V1
  setGpioLow(5); // V2
}

void HamShield::setTxBand70cm() {
  //setGpioHi(4); // V1
  //setGpioHi(5); // V2
  
  uint16_t mode_len = 4;
  uint16_t bit = 11;

  HSwriteBitsW(devAddr, A1846S_GPIO_MODE_REG, bit, mode_len, 0xF);
}

// clk mode
// 12-14MHz: set to 1
// 24-28MHz: set to 0
void HamShield::setClkMode(bool LFClk){
    // include upper bits as default values
    uint16_t tx_data = 0x0FD1;
    if (!LFClk) {
      tx_data = 0x0FD0;  
    }
  
    HSwriteWord(devAddr, A1846S_CLK_MODE_REG, tx_data);
}
bool HamShield::getClkMode(){
    HSreadBitW(devAddr, A1846S_CLK_MODE_REG, A1846S_CLK_MODE_BIT, radio_i2c_buf);
    return (radio_i2c_buf[0] != 0);
}

// TODO: create a 25kHz setup option as well as 12.5kHz (as is implemented now)
/*
// channel mode
// 11 - 25kHz channel
// 00 - 12.5kHz channel
// 10,01 - reserved
void HamShield::setChanMode(uint16_t mode){
    HSwriteBitsW(devAddr, A1846S_CTL_REG, A1846S_CHAN_MODE_BIT, A1846S_CHAN_MODE_LENGTH, mode);
}
uint16_t HamShield::getChanMode(){
    HSreadBitsW(devAddr, A1846S_CTL_REG, A1846S_CHAN_MODE_BIT, A1846S_CHAN_MODE_LENGTH, radio_i2c_buf);
    return radio_i2c_buf[0];
}
*/

// choose tx or rx
void HamShield::setTX(bool on_noff){
    // make sure RX is off
    if (on_noff) {
        tx_active = true;
        rx_active = false;
        setRX(false);
        
        
        if((radio_frequency >= 134000) && (radio_frequency <= 174000)) { 
            setTxBand2m();
        }
        if((radio_frequency >= 200000) && (radio_frequency <= 260000)) { 
            setTxBand1_2m();
        }
        if((radio_frequency >= 400000) && (radio_frequency <= 520000)) { 
            setTxBand70cm();
        }
        // FOR HS03
        //setGpioLow(5); // V2
        //setGpioHi(4); // V1

        
        HSdelay(50); // delay required by AU1846
    }

    HSwriteBitW(devAddr, A1846S_CTL_REG, A1846S_TX_MODE_BIT, on_noff);
}
bool HamShield::getTX(){
    HSreadBitW(devAddr, A1846S_CTL_REG, A1846S_TX_MODE_BIT, radio_i2c_buf);
    return (radio_i2c_buf[0] != 0);
}

void HamShield::setRX(bool on_noff){
    // make sure TX is off
    if (on_noff) {
        tx_active = false;
        rx_active = true;
        setTX(false);
        // FOR HS03
        //setGpioLow(4); // V1
        //setGpioHi(5); // V2
        setGpioLow(4); // V1
        setGpioLow(5); // V2
        
        HSdelay(50); // delay required by AU1846
    }
  
    HSwriteBitW(devAddr, A1846S_CTL_REG, A1846S_RX_MODE_BIT, on_noff);
}
bool HamShield::getRX(){
    HSreadBitW(devAddr, A1846S_CTL_REG, A1846S_RX_MODE_BIT, radio_i2c_buf);
    return (radio_i2c_buf[0] != 0);
}

void HamShield::setModeTransmit(){
    // check to see if we should allow them to do this
    if(restrictions == true) { 
       if(((radio_frequency > 139999) & (radio_frequency < 148001)) || 
          ((radio_frequency > 218999) & (radio_frequency < 225001)) || 
          ((radio_frequency > 419999) & (radio_frequency < 450001)))
        { // we're good, so just drop down to the rest of this function
        } else {
            setRX(false);
            return;
        }            
    }
    setTX(true);
} 
void HamShield::setModeReceive(){
    // turn on rx, turn off tx
    setRX(true);
} 
void HamShield::setModeOff(){
    // turn off tx/rx
    HSwriteBitsW(devAddr, A1846S_CTL_REG, 6, 2, 0);
    
    // turn off amplifiers
    setGpioLow(4); // V1
    setGpioLow(5); // V2

    tx_active = false;
    rx_active = true;
    
    //TODO: set pwr_dwn bit
}

// set tx source
// 000 - Nothing
// 001 - sine source from tone1
// 010 - sine source from tone2
// 011 - sine source from tone1 and tone2
// 100 - mic
void HamShield::setTxSource(uint16_t tx_source){
    HSwriteBitsW(devAddr, A1846S_TX_VOICE_REG, A1846S_VOICE_SEL_BIT, A1846S_VOICE_SEL_LENGTH, tx_source);
}
void HamShield::setTxSourceMic(){
    setTxSource(4);
}
void HamShield::setTxSourceTone1(){
    setTxSource(1);
}
void HamShield::setTxSourceTone2(){
    setTxSource(2);
}
void HamShield::setTxSourceTones(){
    setTxSource(3);
}
void HamShield::setTxSourceNone(){
    setTxSource(0);
}
uint16_t HamShield::getTxSource(){
    HSreadBitsW(devAddr, A1846S_TX_VOICE_REG, A1846S_VOICE_SEL_BIT, A1846S_VOICE_SEL_LENGTH, radio_i2c_buf);
    return radio_i2c_buf[0];
}

/*
// set PA_bias voltage
//    000000: 1.01V
//    000001:1.05V
//    000010:1.09V
//    000100: 1.18V
//    001000: 1.34V
//    010000: 1.68V
//    100000: 2.45V
//    1111111:3.13V
void HamShield::setPABiasVoltage(uint16_t voltage){
    HSwriteBitsW(devAddr, A1846S_PABIAS_REG, A1846S_PABIAS_BIT, A1846S_PABIAS_LENGTH, voltage);
}
uint16_t HamShield::getPABiasVoltage(){
    HSreadBitsW(devAddr, A1846S_PABIAS_REG, A1846S_PABIAS_BIT, A1846S_PABIAS_LENGTH, radio_i2c_buf);
    return radio_i2c_buf[0];
}
*/
// Subaudio settings


// TX and RX code

//   Ctcss/cdcss mode sel
//      00000= disable,
//      00001= det ctcss tone 1
//      00010= det cdcss 
//      00100= det inverted ctcss
//      01000= det ctcss tone 2 (unused in HS right now)
//      10000= det phase shift
void HamShield::setCtcssCdcssMode(uint16_t mode){
    HSwriteBitsW(devAddr, A1846S_TX_VOICE_REG, A1846S_CTDCSS_DTEN_BIT, A1846S_CTDCSS_DTEN_LEN, mode);
}
uint16_t HamShield::getCtcssCdcssMode(){
    HSreadBitsW(devAddr, A1846S_TX_VOICE_REG, A1846S_CTDCSS_DTEN_BIT, A1846S_CTDCSS_DTEN_BIT, radio_i2c_buf);
    return radio_i2c_buf[0];
}

void HamShield::setDetPhaseShift() {
    setCtcssCdcssMode(0x10);
}
void HamShield::setDetInvertCdcss() {
    setCtcssCdcssMode(0x4);
}
void HamShield::setDetCdcss() {
    setCtcssCdcssMode(0x2);
}
void HamShield::setDetCtcss() {
    setCtcssCdcssMode(0x1);
}
		
void HamShield::disableCtcssCdcss(){
    setCtcssCdcssMode(0);
}

//   Ctcss_sel
//      1 = ctcss_cmp/cdcss_cmp out via gpio
//      0 = ctcss/cdcss sdo out via gpio
void HamShield::setCtcssGpioSel(bool cmp_nsdo){
	setGpioFcn(0);
}
bool HamShield::getCtcssGpioSel(){
    uint16_t mode = getGpioMode(0);
	return (mode == 1);
}

//   Cdcss_sel
//      1 = long (24 bit) code
//      0 = short(23 bit) code
void HamShield::setCdcssSel(bool long_nshort){
    HSwriteBitW(devAddr, A1846S_CTCSS_MODE_REG, A1846S_CDCSS_SEL_BIT, long_nshort);
}
bool HamShield::getCdcssSel(){
    HSreadBitW(devAddr, A1846S_CTCSS_MODE_REG, A1846S_CDCSS_SEL_BIT, radio_i2c_buf);
    return (radio_i2c_buf[0] == 1);
}

void HamShield::setCdcssInvert(bool invert) {
	HSwriteBitW(devAddr, A1846S_CTCSS_MODE_REG, A1846S_CDCSS_INVERT_BIT, invert);
}
bool HamShield::getCdcssInvert() {
    HSreadBitW(devAddr, A1846S_CTCSS_MODE_REG, A1846S_CDCSS_INVERT_BIT, radio_i2c_buf);
    return (radio_i2c_buf[0] == 1);
}

// Cdcss neg_det_en
bool HamShield::getCdcssNegDetEnabled(){
	uint16_t css_mode = getCtcssCdcssMode();
	return (css_mode == 4);
}

// Cdcss pos_det_en
bool HamShield::getCdcssPosDetEnabled(){
	uint16_t css_mode = getCtcssCdcssMode();
	return (css_mode == 2);
}

// css_det_en
bool HamShield::getCtssDetEnabled(){
	uint16_t css_mode = getCtcssCdcssMode();
	return (css_mode == 1);
}

// ctcss freq
void HamShield::setCtcss(float freq_Hz) {
    setCtcssFreq((uint16_t) (freq_Hz*100));
}

void HamShield::setCtcssFreq(uint16_t freq_milliHz){
	// set RX Ctcss match thresholds (based on frequency)
	// calculate thresh based on freq
	float f = ((float) freq_milliHz)/100;
	uint8_t thresh = (uint8_t)(-0.1*f + 25);
	setCtcssDetThreshIn(thresh);
	setCtcssDetThreshOut(thresh);
	
	HSwriteWord(devAddr, A1846S_CTCSS_FREQ_REG, freq_milliHz);
}
uint16_t HamShield::getCtcssFreqMilliHz(){
    return getCtcssFreqHz()*100;
}
float HamShield::getCtcssFreqHz() {
	//y = mx + b
	float m = 1.678;
	float b = -3.3;
    HSreadWord(devAddr, A1846S_CTCSS_FREQ_REG, radio_i2c_buf);
	float f = (float) radio_i2c_buf[0];
	return (f/m-b)/100; 
}

void HamShield::setCtcssFreqToStandard(){
    // freq must be 134.4Hz for standard cdcss mode
    setCtcssFreq(13440);
}

void HamShield::enableCtcssTx() {
    HSwriteBitsW(devAddr, A1846S_CTCSS_MODE_REG, 10, 2, 3);
}

void HamShield::enableCtcssRx() {
    setCtcssGpioSel(1);
	HSwriteBitW(devAddr, A1846S_TX_VOICE_REG, A1846S_CTCSS_DET_BIT, 0);
	HSwriteBitW(devAddr, A1846S_FILTER_REG, A1846S_CTCSS_FILTER_BYPASS, 0);
	setDetCtcss();
}

void HamShield::enableCtcss() {	
	// enable TX
	enableCtcssTx();
    
	// enable RX
	enableCtcssRx();
}

void HamShield::disableCtcssTx() {
    HSwriteBitsW(devAddr, A1846S_CTCSS_MODE_REG, 10, 2, 0);
}

void HamShield::disableCtcssRx() {
    setCtcssGpioSel(0);
	disableCtcssCdcss();
}
void HamShield::disableCtcss() {
	disableCtcssTx();
    disableCtcssRx();
}

// match threshold
void HamShield::setCtcssDetThreshIn(uint8_t thresh) {
	HSwriteBitsW(devAddr, A1846S_CTCSS_THRESH_REG, 15, 8, thresh);
}
uint8_t HamShield::getCtcssDetThreshIn() {
	HSreadBitsW(devAddr, A1846S_CTCSS_THRESH_REG, 15, 8, radio_i2c_buf);
	return (uint8_t) radio_i2c_buf[0];
}

// unmatch threshold
void HamShield::setCtcssDetThreshOut(uint8_t thresh) {
	HSwriteBitsW(devAddr, A1846S_CTCSS_THRESH_REG, 7, 8, thresh);
}
uint8_t HamShield::getCtcssDetThreshOut() {
	HSreadBitsW(devAddr, A1846S_CTCSS_THRESH_REG, 7, 8, radio_i2c_buf);
	return (uint8_t) radio_i2c_buf[0];
}

bool HamShield::getCtcssToneDetected() {
	HSreadBitW(devAddr, A1846S_FLAG_REG, A1846S_CTCSS1_FLAG_BIT, radio_i2c_buf);
	return (radio_i2c_buf[0] != 0);
}

// cdcss codes
void HamShield::setCdcssCode(uint16_t code) {
    // note: assuming a well formed code (xyz, where x, y, and z are all 0-7)

    // Set both code registers at once (23 or 24 bit code)
    // sends 100, c1, c2, c3, 11 bits of crc
    
    // TODO: figure out what to do about 24 or 23 bit codes
    
    uint32_t cdcss_code = 0x800000; // top three bits are 100
    uint32_t oct_code = code%10;
    code = code / 10;
    cdcss_code += oct_code << 20;
    oct_code = code % 10;
    code = code / 10;
    cdcss_code += oct_code << 17;
    cdcss_code += (code % 10) << 14;
    
    // TODO: CRC
    
    // set registers
    uint16_t temp_code = (uint16_t) cdcss_code;
    HSwriteWord(devAddr, A1846S_CDCSS_CODE_LO_REG, temp_code);
    temp_code = ((uint16_t) (cdcss_code >> 16))&0x00FF;    
    HSwriteWord(devAddr, A1846S_CDCSS_CODE_HI_REG, temp_code);    
}
uint16_t HamShield::getCdcssCode() {
    uint32_t oct_code;
    HSreadWord(devAddr, A1846S_CDCSS_CODE_HI_REG, radio_i2c_buf);
    oct_code = ((uint32_t)radio_i2c_buf[0] << 16);
    HSreadWord(devAddr, A1846S_CDCSS_CODE_LO_REG, radio_i2c_buf);
    oct_code += radio_i2c_buf[0];
    
    oct_code = oct_code >> 12;
    uint16_t code = (oct_code & 0x3);
    oct_code = oct_code >> 3;
    code += (oct_code & 0x3)*10;
    oct_code = oct_code >> 3;
    code += (oct_code & 0x3)*100;
    
    return code;
}

// SQ
void HamShield::setSQOn(){
    HSwriteBitW(devAddr, A1846S_CTL_REG, A1846S_SQ_ON_BIT, 1);
}
void HamShield::setSQOff(){
    HSwriteBitW(devAddr, A1846S_CTL_REG, A1846S_SQ_ON_BIT, 0);
}
bool HamShield::getSQState(){
    HSreadBitW(devAddr, A1846S_CTL_REG, A1846S_SQ_ON_BIT, radio_i2c_buf);
    return (radio_i2c_buf[0] != 0);
}

// SQ threshold
void HamShield::setSQHiThresh(int16_t sq_hi_threshold){
    // Sq detect high th, rssi_cmp will be 1 when rssi>th_h_sq, unit 1dB
    uint16_t sq = 137 + sq_hi_threshold;
    HSwriteBitsW(devAddr, A1846S_SQ_OPEN_THRESH_REG, A1846S_SQ_OPEN_THRESH_BIT, A1846S_SQ_OPEN_THRESH_LENGTH, sq);
} 
int16_t HamShield::getSQHiThresh(){
    HSreadBitsW(devAddr, A1846S_SQ_OPEN_THRESH_REG, A1846S_SQ_OPEN_THRESH_BIT, A1846S_SQ_OPEN_THRESH_LENGTH, radio_i2c_buf);

    return radio_i2c_buf[0] - 137;
}
void HamShield::setSQLoThresh(int16_t sq_lo_threshold){
    // Sq detect low th, rssi_cmp will be 0 when rssi<th_l_sq && time delay meet, unit 1 dB
    uint16_t sq = 137 + sq_lo_threshold;
    HSwriteBitsW(devAddr, A1846S_SQ_SHUT_THRESH_REG, A1846S_SQ_SHUT_THRESH_BIT, A1846S_SQ_SHUT_THRESH_LENGTH, sq);
}
int16_t HamShield::getSQLoThresh(){
    HSreadBitsW(devAddr, A1846S_SQ_SHUT_THRESH_REG, A1846S_SQ_SHUT_THRESH_BIT, A1846S_SQ_SHUT_THRESH_LENGTH, radio_i2c_buf);

    return radio_i2c_buf[0] - 137;
}

bool HamShield::getSquelching() {
    HSreadBitW(devAddr, A1846S_FLAG_REG, A1846S_SQ_FLAG_BIT, radio_i2c_buf);
    return (radio_i2c_buf[0] != 0);
}


// SQ out select
void HamShield::setSQOutSel(){
    HSwriteBitW(devAddr, A1846S_SQ_OUT_SEL_REG, A1846S_SQ_OUT_SEL_BIT, 1);
}
void HamShield::clearSQOutSel(){
    HSwriteBitW(devAddr, A1846S_SQ_OUT_SEL_REG, A1846S_SQ_OUT_SEL_BIT, 0);
}
bool HamShield::getSQOutSel(){
    HSreadBitW(devAddr, A1846S_SQ_OUT_SEL_REG, A1846S_SQ_OUT_SEL_BIT, radio_i2c_buf);
    return (radio_i2c_buf[0] != 0);
}

// VOX
void HamShield::setVoxOn(){
    HSwriteBitW(devAddr, A1846S_CTL_REG, A1846S_VOX_ON_BIT, 1);
}
void HamShield::setVoxOff(){
    HSwriteBitW(devAddr, A1846S_CTL_REG, A1846S_VOX_ON_BIT, 0);
}
bool HamShield::getVoxOn(){
    HSreadBitW(devAddr, A1846S_CTL_REG, A1846S_VOX_ON_BIT, radio_i2c_buf);
    return (radio_i2c_buf[0] != 0);
}

// Vox Threshold
void HamShield::setVoxOpenThresh(uint16_t vox_open_thresh){
    // When vssi > th_h_vox, then vox will be 1(unit mV )
    HSwriteBitsW(devAddr, A1846S_TH_H_VOX_REG, A1846S_TH_H_VOX_BIT, A1846S_TH_H_VOX_LEN, vox_open_thresh);

} 
uint16_t HamShield::getVoxOpenThresh(){
    HSreadBitsW(devAddr, A1846S_TH_H_VOX_REG, A1846S_TH_H_VOX_BIT, A1846S_TH_H_VOX_LEN, radio_i2c_buf);
    return radio_i2c_buf[0];
}
void HamShield::setVoxShutThresh(uint16_t vox_shut_thresh){
    // When vssi < th_l_vox && time delay meet, then vox will be 0 (unit mV )
    HSwriteBitsW(devAddr, A1846S_TH_L_VOX_REG, A1846S_TH_L_VOX_BIT, A1846S_TH_L_VOX_LEN, vox_shut_thresh);
} 
uint16_t HamShield::getVoxShutThresh(){
    HSreadBitsW(devAddr, A1846S_TH_L_VOX_REG, A1846S_TH_L_VOX_BIT, A1846S_TH_L_VOX_LEN, radio_i2c_buf);
    return radio_i2c_buf[0];
}

// Tail Noise
void HamShield::enableTailNoiseElim(){
    HSwriteBitW(devAddr, A1846S_CTL_REG, A1846S_TAIL_ELIM_EN_BIT, 1);
}
void HamShield::disableTailNoiseElim(){
    HSwriteBitW(devAddr, A1846S_CTL_REG, A1846S_TAIL_ELIM_EN_BIT, 1);
}
bool HamShield::getTailNoiseElimEnabled(){
    HSreadBitW(devAddr, A1846S_CTL_REG, A1846S_TAIL_ELIM_EN_BIT, radio_i2c_buf);
    return (radio_i2c_buf[0] != 0);
}

// tail noise shift select
//   Select ctcss phase shift when use tail eliminating function when TX
//     00 = 0 degree shift
//     01 = 120 degree shift
//     10 = 180 degree shift
//     11 = 240 degree shift
void HamShield::setShiftSelect(uint16_t shift_sel){
  HSwriteBitsW(devAddr, A1846S_CTCSS_MODE_REG, A1846S_SHIFT_SEL_BIT, A1846S_SHIFT_SEL_LEN, shift_sel);
}
uint16_t HamShield::getShiftSelect(){
  HSreadBitsW(devAddr, A1846S_CTCSS_MODE_REG, A1846S_SHIFT_SEL_BIT, A1846S_SHIFT_SEL_LEN, radio_i2c_buf);
  return radio_i2c_buf[0];
}

// DTMF
void HamShield::enableDTMFReceive(){
  uint16_t tx_data;

  tx_data = 0x2264;
  HSwriteWord(devAddr, 0x77, tx_data);
  tx_data = 0xD984;
  HSwriteWord(devAddr, 0x78, tx_data);
  tx_data = 0x1E3C;
  HSwriteWord(devAddr, 0x79, tx_data);

  HSwriteBitsW(devAddr, A1846S_DTMF_ENABLE_REG, A1846S_DTMF_ENABLE_BIT, 1, 1);
  
  //HSwriteBitsW(devAddr, 0x57, 0, 1, 1); // send dtmf to speaker out
  
  // bypass pre/de-emphasis
  HSwriteBitsW(devAddr, A1846S_FILTER_REG, A1846S_EMPH_FILTER_EN, 1, 1);
  
}

void HamShield::setDTMFDetectTime(uint16_t detect_time) {
  if (detect_time > 255) {detect_time = 255;} // maxed out
  HSwriteBitsW(devAddr, A1846S_DTMF_ENABLE_REG, A18462_DTMF_DET_TIME_BIT, A18462_DTMF_DET_TIME_LEN, detect_time);
}

uint16_t HamShield::getDTMFDetectTime() {
  HSreadBitsW(devAddr, A1846S_DTMF_ENABLE_REG, A18462_DTMF_DET_TIME_BIT, A18462_DTMF_DET_TIME_LEN, radio_i2c_buf);
  return radio_i2c_buf[0]; 
}

void HamShield::setDTMFIdleTime(uint16_t idle_time) {
  if (idle_time > 63) {idle_time = 63;} // maxed out
  // idle time is time between DTMF Tone
  HSwriteBitsW(devAddr, A1846S_DTMF_TIME_REG, A1846S_DTMF_IDLE_TIME_BIT, A1846S_DTMF_IDLE_TIME_LEN, idle_time);    
}

uint16_t HamShield::getDTMFIdleTime() {
  HSreadBitsW(devAddr, A1846S_DTMF_TIME_REG, A1846S_DTMF_IDLE_TIME_BIT, A1846S_DTMF_IDLE_TIME_LEN, radio_i2c_buf);
  return radio_i2c_buf[0];  
}

char HamShield::DTMFRxLoop() {
  char m = 0;
  if (getDTMFSample() != 0) {
    uint16_t code = getDTMFCode();

    m = DTMFcode2char(code);

    // reset after this tone
    int j = 0;
    while (j < 4) {
      if (getDTMFSample() == 0) {
        j++;
      } else {
        j = 1;
      }
      delay(10);
    }
    // reset read
    //enableDTMFReceive();
  }
  
  return m;
}


char HamShield::DTMFcode2char(uint16_t code) {
  char c;
  if (code < 10) {
    c = '0' + code;
  } else if (code < 0xE) {
    c = 'A' + code - 10;
  } else if (code == 0xE) {
    c = '*';
  } else if (code == 0xF) {
    c = '#';
  } else {
    c = '?'; // invalid code
  }
 
  return c;
}

uint8_t HamShield::DTMFchar2code(char c) {
    uint8_t code;
    if (c == '#') {
      code = 0xF;
    } else if (c=='*') {
      code = 0xE;
    } else if (c >= 'A' && c <= 'D') {
      code = c - 'A' + 0xA;
    } else if (c >= '0' && c <= '9') {
      code = c - '0';
    } else {
      // invalid code, skip it
      code = 255;
    }

    return code;
}


void HamShield::setDTMFTxTime(uint16_t tx_time) {
  if (tx_time > 63) {tx_time = 63;} // maxed out
  // tx time is duration of DTMF Tone
  HSwriteBitsW(devAddr, A1846S_DTMF_TIME_REG, A1846S_DUALTONE_TX_TIME_BIT, A1846S_DUALTONE_TX_TIME_LEN, tx_time);  
}

uint16_t HamShield::getDTMFTxTime() {
  HSreadBitsW(devAddr, A1846S_DTMF_TIME_REG, A1846S_DUALTONE_TX_TIME_BIT, A1846S_DUALTONE_TX_TIME_LEN, radio_i2c_buf);
  return radio_i2c_buf[0];  
}

uint16_t HamShield::disableDTMF(){
  HSwriteBitsW(devAddr, A1846S_DTMF_ENABLE_REG, A1846S_DTMF_ENABLE_BIT, 1, 0);
}

uint16_t HamShield::getDTMFSample(){
  HSreadBitsW(devAddr, A1846S_DTMF_CODE_REG, A1846S_DTMF_SAMPLE_BIT, 1, radio_i2c_buf);
  return radio_i2c_buf[0];
}

uint16_t HamShield::getDTMFTxActive(){
  HSreadBitsW(devAddr, A1846S_DTMF_CODE_REG, A1846S_DTMF_TX_IDLE_BIT, 1, radio_i2c_buf);
  return radio_i2c_buf[0];
}

uint16_t HamShield::getDTMFCode(){
  HSreadBitsW(devAddr, A1846S_DTMF_CODE_REG, A1846S_DTMF_CODE_BIT, A1846S_DTMF_CODE_LEN, radio_i2c_buf);
  return radio_i2c_buf[0];
}

void HamShield::setDTMFCode(uint16_t code){
  uint16_t tone1, tone2;

  /*
   *     F4    F5    F6    F7
   * F0   1     2     3     A
   * F1   4     5     6     B
   * F2   7     8     9     C
   * F3   E(*)  0   F(#)    D
   */

  // determine tone 1
  if ((code >= 1 && code <= 3) || code == 0xA) {
      tone1 = 697*10;
  } else if ((code >= 4 && code <= 6) || code == 0xB) {
      tone1 = 770*10;
  } else if ((code >= 7 && code <= 9) || code == 0xC) {
      tone1 = 852*10;
  } else if (code >= 0xD || code == 0) {
      tone1 = 941*10;
  }

  // determine tone 2
  if (code == 1 || code == 4 || code == 7 || code == 0xE) {
      tone2 = 1209*10;
  } else if (code == 2 || code == 5 || code == 8 || code == 0) {
      tone2 = 1336*10;
  } else if (code == 3 || code == 6 || code == 9 || code == 0xF) {
      tone2 = 1477*10;
  } else if (code >= 0xA && code <= 0xD) {
      tone2 = 1633*10;
  }
  
  HSwriteWord(devAddr, A1846S_TONE1_FREQ, tone1);
  HSwriteWord(devAddr, A1846S_TONE2_FREQ, tone2);

}

// Tone Transmission


void HamShield::HStone(uint8_t pin, unsigned int frequency) {
  // store old dtmf reg for noTone
//  HSreadWord(devAddr, A1846S_DTMF_ENABLE_REG, radio_i2c_buf);
//  old_dtmf_reg = radio_i2c_buf[0];   
  
  // set frequency
  HSwriteWord(devAddr, A1846S_TONE1_FREQ, frequency*10);
  
  // set 0x79 dtmf control
  HSwriteBitsW(devAddr, 0x79, 15, 2, 0x3); // transmit single tone (not dtmf)
//  HSwriteBitsW(devAddr, A1846S_DTMF_ENABLE_REG, A1846S_DTMF_ENABLE_BIT, 2, 0x2); // transmit single tone (not dtmf)
  
  // bypass pre/de-emphasis
  HSwriteBitsW(devAddr, A1846S_FILTER_REG, A1846S_EMPH_FILTER_EN, 1, 1);

  // set source for tx
  setTxSourceTone1(); // writes to 3A

  //tone(pin, frequency);
}
void HamShield::HSnoTone(uint8_t pin) {
  setTxSourceMic();
  //HSwriteWord(devAddr, A1846S_DTMF_ENABLE_REG, old_dtmf_reg); // disable tone and dtmf
//  noTone(pin);
}

// Tone detection
void HamShield::lookForTone(uint16_t t_hz) {
    // set 0x79 dtmf control
    HSwriteBitsW(devAddr, 0x79, 15, 2, 0x3); // transmit single tone (not dtmf)
  
    // bypass pre/de-emphasis
    HSwriteBitsW(devAddr, A1846S_FILTER_REG, A1846S_EMPH_FILTER_EN, 1, 1);

    float tone_hz = (float) t_hz;
	float Fs = 6400000/1024;
	float k = floor(tone_hz/Fs*127 + 0.5);
	uint16_t t = (uint16_t) (round(2.0*cos(2.0*M_PI*k/127)*1024));
	
	float k2 = floor(2*tone_hz/Fs*127+0.5);
	uint16_t h = (uint16_t) (round(2.0*cos(2.0*M_PI*k2/127)*1024));
	// set tone
	HSwriteWord(devAddr, 0x67, t); // looking for tone 1
	
	// set second harmonic
	HSwriteWord(devAddr, 0x6F, h); // looking for tone 
    
	// turn on tone detect
	HSwriteBitW(devAddr, A1846S_DTMF_ENABLE_REG, A1846S_TONE_DETECT, 1);
	HSwriteBitW(devAddr, A1846S_DTMF_ENABLE_REG, A1846S_DTMF_ENABLE_BIT, 1);

}

bool redetect = false;
uint8_t last_tone_detected = 0;
uint8_t HamShield::toneDetected() {
	HSreadBitsW(devAddr, A1846S_DTMF_CODE_REG, A1846S_DTMF_SAMPLE_BIT, 1, radio_i2c_buf);
	if (radio_i2c_buf[0] != 0) {
      if (!redetect) {
        redetect = true;
		HSreadBitsW(devAddr, A1846S_DTMF_CODE_REG, A1846S_DTMF_CODE_BIT, A1846S_DTMF_CODE_LEN, radio_i2c_buf);
		last_tone_detected = radio_i2c_buf[0];
        //Serial.print("t: ");
        //Serial.println(last_tone_detected);
      }
      if (last_tone_detected == 0) {
        return 1;
      }
	} else if (redetect) {
      // re-enable detect
      redetect = false;
      HSwriteBitW(devAddr, A1846S_DTMF_ENABLE_REG, A1846S_TONE_DETECT, 1);
      HSwriteBitW(devAddr, A1846S_DTMF_ENABLE_REG, A1846S_DTMF_ENABLE_BIT, 1);
    }
	return 0;
}

// TX FM deviation
void HamShield::setFMVoiceCssDeviation(uint16_t deviation){
    HSwriteBitsW(devAddr, A1846S_FM_DEV_REG, A1846S_FM_DEV_VOICE_BIT, A1846S_FM_DEV_VOICE_LENGTH, deviation);
}
uint16_t HamShield::getFMVoiceCssDeviation(){
    HSreadBitsW(devAddr, A1846S_FM_DEV_REG, A1846S_FM_DEV_VOICE_BIT, A1846S_FM_DEV_VOICE_LENGTH, radio_i2c_buf);
    return radio_i2c_buf[0];
}
void HamShield::setFMCssDeviation(uint16_t deviation){
    HSwriteBitsW(devAddr, A1846S_FM_DEV_REG, A1846S_FM_DEV_CSS_BIT, A1846S_FM_DEV_CSS_LENGTH, deviation);
}
uint16_t HamShield::getFMCssDeviation(){
    HSreadBitsW(devAddr, A1846S_FM_DEV_REG, A1846S_FM_DEV_CSS_BIT, A1846S_FM_DEV_CSS_LENGTH, radio_i2c_buf);
    return radio_i2c_buf[0];
}

// RX voice range
void HamShield::setMute() {
	HSwriteBitW(devAddr, A1846S_CTL_REG, A1846S_MUTE_BIT, 1);
}
void HamShield::setUnmute() {
	HSwriteBitW(devAddr, A1846S_CTL_REG, A1846S_MUTE_BIT, 0);
}

void HamShield::setVolume1(uint16_t volume){
    HSwriteBitsW(devAddr, A1846S_RX_VOLUME_REG, A1846S_RX_VOL_1_BIT, A1846S_RX_VOL_1_LENGTH, volume);
}
uint16_t HamShield::getVolume1(){
    HSreadBitsW(devAddr, A1846S_RX_VOLUME_REG, A1846S_RX_VOL_1_BIT, A1846S_RX_VOL_1_LENGTH, radio_i2c_buf);
    return radio_i2c_buf[0];
}
void HamShield::setVolume2(uint16_t volume){
    HSwriteBitsW(devAddr, A1846S_RX_VOLUME_REG, A1846S_RX_VOL_2_BIT, A1846S_RX_VOL_2_LENGTH, volume);
}
uint16_t HamShield::getVolume2(){
    HSreadBitsW(devAddr, A1846S_RX_VOLUME_REG, A1846S_RX_VOL_2_BIT, A1846S_RX_VOL_2_LENGTH, radio_i2c_buf);
    return radio_i2c_buf[0];
}

// GPIO
void HamShield::setGpioMode(uint16_t gpio, uint16_t mode){
    uint16_t mode_len = 2;
    uint16_t bit = gpio*2 + 1;

    HSwriteBitsW(devAddr, A1846S_GPIO_MODE_REG, bit, mode_len, mode);
}
void HamShield::setGpioHiZ(uint16_t gpio){
    setGpioMode(gpio, 0);
}
void HamShield::setGpioFcn(uint16_t gpio){
    setGpioMode(gpio, 1);
}
void HamShield::setGpioLow(uint16_t gpio){
    setGpioMode(gpio, 2);
}
void HamShield::setGpioHi(uint16_t gpio){
    setGpioMode(gpio, 3);
}
uint16_t HamShield::getGpioMode(uint16_t gpio){
    uint16_t mode_len = 2;
    uint16_t bit = gpio*2 + 1;
    
    HSreadBitsW(devAddr, A1846S_GPIO_MODE_REG, bit, mode_len, radio_i2c_buf);
    return radio_i2c_buf[0];
}

void HamShield::setGpios(uint16_t mode){
    HSwriteWord(devAddr, A1846S_GPIO_MODE_REG, mode);
}

uint16_t HamShield::getGpios(){
    HSreadWord(devAddr, A1846S_GPIO_MODE_REG, radio_i2c_buf);
    return radio_i2c_buf[0];
}

// Int
void HamShield::enableInterrupt(uint16_t interrupt){
    HSwriteBitW(devAddr, A1846S_INT_MODE_REG, interrupt, 1);
}
void HamShield::disableInterrupt(uint16_t interrupt){
    HSwriteBitW(devAddr, A1846S_INT_MODE_REG, interrupt, 0);
}
bool HamShield::getInterruptEnabled(uint16_t interrupt){
    HSreadBitW(devAddr, A1846S_INT_MODE_REG, interrupt, radio_i2c_buf);
    return (radio_i2c_buf[0] != 0);
}

// ST mode
void HamShield::setStMode(uint16_t mode){
    HSwriteBitsW(devAddr, A1846S_CTL_REG, A1846S_ST_MODE_BIT, A1846S_ST_MODE_LENGTH, mode);
}
uint16_t HamShield::getStMode(){
    HSreadBitsW(devAddr, A1846S_CTL_REG, A1846S_ST_MODE_BIT, A1846S_ST_MODE_LENGTH, radio_i2c_buf);
    return radio_i2c_buf[0];
}
void HamShield::setStFullAuto(){
setStMode(2);
}
void HamShield::setStRxAutoTxManu(){
setStMode(1);
}
void HamShield::setStFullManu(){
setStMode(0);
}

// Pre-emphasis, De-emphasis filter
void HamShield::bypassPreDeEmph(){
    HSwriteBitW(devAddr, A1846S_FILTER_REG, A1846S_EMPH_FILTER_EN, 1);
}
void HamShield::usePreDeEmph(){
    HSwriteBitW(devAddr, A1846S_FILTER_REG, A1846S_EMPH_FILTER_EN, 0);
}
bool HamShield::getPreDeEmphEnabled(){
    HSreadBitW(devAddr, A1846S_FILTER_REG, A1846S_EMPH_FILTER_EN, radio_i2c_buf);
    return (radio_i2c_buf[0] == 0);
}

// Voice Filters
void HamShield::bypassVoiceHpf(){
    HSwriteBitW(devAddr, A1846S_FILTER_REG, A1846S_VHPF_FILTER_EN, 1);
}
void HamShield::useVoiceHpf(){
    HSwriteBitW(devAddr, A1846S_FILTER_REG, A1846S_VHPF_FILTER_EN, 0);
}
bool HamShield::getVoiceHpfEnabled(){
    HSreadBitW(devAddr, A1846S_FILTER_REG, A1846S_VHPF_FILTER_EN, radio_i2c_buf);
    return (radio_i2c_buf[0] == 0);
}

void HamShield::bypassVoiceLpf(){
    HSwriteBitW(devAddr, A1846S_FILTER_REG, A1846S_VLPF_FILTER_EN, 1);
}
void HamShield::useVoiceLpf(){
    HSwriteBitW(devAddr, A1846S_FILTER_REG, A1846S_VLPF_FILTER_EN, 0);
}
bool HamShield::getVoiceLpfEnabled(){
    HSreadBitW(devAddr, A1846S_FILTER_REG, A1846S_VLPF_FILTER_EN, radio_i2c_buf);
    return (radio_i2c_buf[0] == 0);
}

// Vox filters

void HamShield::bypassVoxHpf(){
    HSwriteBitW(devAddr, A1846S_FILTER_REG, A1846S_VXHPF_FILTER_EN, 1);
}
void HamShield::useVoxHpf(){
    HSwriteBitW(devAddr, A1846S_FILTER_REG, A1846S_VXHPF_FILTER_EN, 0);
}
bool HamShield::getVoxHpfEnabled(){
    HSreadBitW(devAddr, A1846S_FILTER_REG, A1846S_VXHPF_FILTER_EN, radio_i2c_buf);
    return (radio_i2c_buf[0] == 0);
}

void HamShield::bypassVoxLpf(){
    HSwriteBitW(devAddr, A1846S_FILTER_REG, A1846S_VXLPF_FILTER_EN, 1);
}
void HamShield::useVoxLpf(){
    HSwriteBitW(devAddr, A1846S_FILTER_REG, A1846S_VXLPF_FILTER_EN, 0);
}
bool HamShield::getVoxLpfEnabled(){
    HSreadBitW(devAddr, A1846S_FILTER_REG, A1846S_VXLPF_FILTER_EN, radio_i2c_buf);
    return (radio_i2c_buf[0] == 0);
}



// Read Only Status Registers
int16_t HamShield::readRSSI(){
    HSreadBitsW(devAddr, A1846S_RSSI_REG, A1846S_RSSI_BIT, A1846S_RSSI_LENGTH, radio_i2c_buf);
    
    int16_t rssi = (radio_i2c_buf[0] & 0xFF) - 137;
    return rssi;
}
uint16_t HamShield::readVSSI(){
    HSreadBitsW(devAddr, A1846S_VSSI_REG, A1846S_VSSI_BIT, A1846S_VSSI_LENGTH, radio_i2c_buf);
    
    return radio_i2c_buf[0];
}
uint16_t HamShield::readMSSI(){
    HSreadBitsW(devAddr, A1846S_VSSI_REG, A1846S_MSSI_BIT, A1846S_MSSI_LENGTH, radio_i2c_buf);
    
    return radio_i2c_buf[0];
}


void HamShield::setRfPower(uint8_t pwr) {
    int max_pwr = 15;
    if (pwr > max_pwr) {
        pwr = max_pwr; 
    }

    // turn off tx/rx
    HSwriteBitsW(devAddr, A1846S_CTL_REG, 6, 2, 0);
   
    HSwriteBitsW(devAddr, A1846S_PABIAS_REG, A1846S_PADRV_BIT, A1846S_PADRV_LENGTH, pwr);

       if (rx_active) {
        setRX(true);
    } else if (tx_active) {
        setTX(true);
    }
}



bool HamShield::frequency(uint32_t freq_khz) {  

  if((freq_khz >= 134000) && (freq_khz <= 174000)) { 
      setTxBand2m();
      setFrequency(freq_khz);
      return true;
  }
  
  if((freq_khz >= 200000) && (freq_khz <= 260000)) { 
      setTxBand1_2m();
      setFrequency(freq_khz);
      return true;
  }
  
  if((freq_khz >= 400000) && (freq_khz <= 520000)) { 
      setTxBand70cm();
      setFrequency(freq_khz); 
      return true;
  }
  return false;
}

bool HamShield::frequency_float(float freq_khz) {
    
    if((freq_khz >= 134000) && (freq_khz <= 174000)) { 
      setTxBand2m();
    } else if((freq_khz >= 200000) && (freq_khz <= 260000)) { 
      setTxBand1_2m();
    } else if((freq_khz >= 400000) && (freq_khz <= 520000)) { 
      setTxBand70cm();
    } else {
      return false;
    }
    
    // convert from float to int
    uint32_t freq_raw = (uint32_t) (freq_khz * 16); // radio_frequency is accurate to 1/16 kHz
    radio_frequency = ((float) freq_raw) / 16; // radio_frequency is accurate to 1/16 kHz
    
    // turn off tx/rx
    HSwriteBitsW(devAddr, A1846S_CTL_REG, 6, 2, 0);
    
    // if we're using a 12MHz crystal and the frequency is
    // 136.5M,409.5M and 455M, then we have to do special stuff
    if (radio_frequency == 136500 ||
        radio_frequency == 490500 ||
        radio_frequency == 455000) {
        
        // set up AU1846 for funky freq
        HSwriteWord(devAddr, 0x05, 0x86D3);

    } else {
        // set up AU1846 for normal freq
        HSwriteWord(devAddr, 0x05, 0x8763);
    }
    
    // send top 16 bits to A1846S_FREQ_HI_REG    
    uint16_t freq_half = (uint16_t) (0x3FFF & (freq_raw >> 16));
    HSwriteWord(devAddr, A1846S_FREQ_HI_REG, freq_half);
    //  send bottom 16 bits to A1846S_FREQ_LO_REG
    freq_half = (uint16_t) (freq_raw & 0xFFFF);
    HSwriteWord(devAddr, A1846S_FREQ_LO_REG, freq_half);
    
     if (rx_active) {
        setRX(true);
    } else if (tx_active) {
        setTX(true);
    }
}


/* FRS Lookup Table */

bool HamShield::setFRSChannel(uint8_t channel) { 
  if(channel < 15) {
#if defined(__AVR__)
    setFrequency(pgm_read_dword_near(FRS + channel));
#else
    setFrequency(FRS[channel]); 
#endif
    return true;
  }
  return false;
} 

/* GMRS Lookup Table (borrows from FRS table since channels overlap) */

bool HamShield::setGMRSChannel(uint8_t channel) { 
  if((channel > 8) & (channel < 16)) { 
     channel = channel - 7;           // we start with 0, to try to avoid channel 8 being nothing
#if defined(__AVR__)
    setFrequency(pgm_read_dword_near(FRS + channel));
#else
    setFrequency(FRS[channel]); 
#endif
     return true; 
  }
  if(channel < 9) { 
#if defined(__AVR__)
    setFrequency(pgm_read_dword_near(GMRS + channel));
#else
    setFrequency(GMRS[channel]); 
#endif
     return true;
  }
  return false;
}

/* MURS band is 11.25KHz (2.5KHz dev) in channel 1-3, 20KHz (5KHz dev) in channel 4-5. Should we set this? */

bool HamShield::setMURSChannel(uint8_t channel) { 
  if(channel < 6) { 
#if defined(__AVR__)
    setFrequency(pgm_read_dword_near(MURS + channel));
#else
    setFrequency(MURS[channel]); 
#endif     
     return true;
  }
}

/* Weather radio channels */

bool HamShield::setWXChannel(uint8_t channel) { 
  if(channel < 8) { 
#if defined(__AVR__)
    setFrequency(pgm_read_dword_near(WX + channel));
#else
    setFrequency(WX[channel]); 
#endif
     setModeReceive();
     // turn off squelch?
     // channel bandwidth? 
     return true;
  }
  return false;
}

/* Scan channels for strongest signal. returns channel number. You could do radio.setWXChannel(radio.scanWXChannel()) */

uint8_t HamShield::scanWXChannel() { 
  uint8_t channel = 0;
  int16_t toprssi = 0;
  for(int x = 0; x < 8; x++) { 
     setWXChannel(x);
     HSdelay(100);
     int16_t rssi = readRSSI();
     if(rssi > toprssi) { toprssi = rssi; channel = x; }
  }
  return channel;
}


/* removes the out of band transmit restrictions for those who hold special licenses */

void HamShield::dangerMode() { 
  restrictions = false;
  return;
} 

/* enable restrictions on out of band transmissions */

void HamShield::safeMode() { 
  restrictions = true;
  return;
}

/* scanner mode. Scans a range and returns the active frequency when it detects a signal. If none is detected, returns 0. */

uint32_t HamShield::scanMode(uint32_t start,uint32_t stop, uint8_t speed, uint16_t step, uint16_t threshold) { 
    setModeReceive();
    int16_t rssi = -150;
    for(uint32_t freq = start; freq < stop; freq = freq + step) { 
        setFrequency(freq);
        for(int x = 0; x < speed; x++) { 
            rssi = readRSSI();
            if(rssi > threshold) { return freq; } 
        }
    }
    return 0;  // found nothing
}

/* white space finder. (inverted scanner) Scans a range for a white space, and if no signal exists, stop there. */

uint32_t HamShield::findWhitespace(uint32_t start,uint32_t stop, uint8_t dwell, uint16_t step, uint16_t threshold) { 
    setModeReceive();
    int16_t rssi = -150;
    for(uint32_t freq = start; freq < stop; freq = freq + step) { 
        setFrequency(freq);
        for(int x = 0; x < dwell; x++) { 
            rssi = readRSSI();
            if(rssi > threshold) { break; } 
        }
        if(rssi < threshold) { return freq; }  /* found a blank channel */
    }
    return 0;  // everything is busy
}

/* 
channel scanner. Scans an array of channels for activity. returns channel number if found. Otherwise, returns 0. ignores whatever is in array position
0  
*/

uint32_t HamShield::scanChannels(uint32_t buffer[],uint8_t buffsize, uint8_t speed, uint16_t threshold) { 
        setModeReceive();
        int16_t rssi = 0;
        for(int x = 1; x < buffsize; x++) { 
               setFrequency(buffer[x]);
               for(int y = 0; y < speed; y++) { 
                  rssi = readRSSI();
                  if(rssi > threshold) { return x; } 
               }
        }               
        return 0;

}

/* 
white space channel finder. Scans an array of channels for white space. returns channel number if empty found. Otherwise, returns 0. ignores whatever is in array position
0  
*/

uint32_t HamShield::findWhitespaceChannels(uint32_t buffer[],uint8_t buffsize, uint8_t dwell, uint16_t threshold) { 
        setModeReceive();
        int16_t rssi = 0;
        for(int x = 1; x < buffsize; x++) { 
               setFrequency(buffer[x]);
        for(int y = 0; y < dwell; y++) { 
            rssi = readRSSI();
            if(rssi > threshold) { break; } 
        }
        if(rssi < threshold) { return x; }  /* found a blank channel */
    }
    return 0;  // everything is busy
}


/* 

 Radio etiquette function: Wait for empty channel.

 Optional timeout (0 waits forever)
 Optional break window (how much dead air to wait for after a transmission completes)

Does not take in account the millis() overflow 
   
*/

bool HamShield::waitForChannel(long timeout = 0, long breakwindow = 0, int setRSSI = HAMSHIELD_EMPTY_CHANNEL_RSSI) { 
    int16_t rssi = 0;                                                              // Set RSSI to max received signal
    for(int x = 0; x < 20; x++) { rssi = readRSSI(); }                            // "warm up" to get past RSSI hysteresis 
    long timer = HSmillis() + timeout;                                              // Setup the timeout value
    if(timeout == 0) { timer = 4294967295; }                                      // If we want to wait forever, set it to the max millis()
    while(timer > HSmillis()) {                                                     // while our timer is not timed out.
        rssi = readRSSI();                                                        // Read signal strength
        if(rssi < setRSSI) {                                 // If the channel is empty, lets see if anyone breaks in.
             timer = HSmillis() + breakwindow;
             while(timer > HSmillis()) {
                 rssi = readRSSI();
                 if(rssi > setRSSI) { return false; }        // Someone broke into the channel, abort.
             } return true;                                                       // It passed the test...channel is open.
        }
    }
    return false;
}

void HamShield::setupMorseRx() {
  // TODO: morse timing config (e.g. dot time, dash time, etc)
}

// Get current morse code tone frequency (in Hz)

unsigned int HamShield::getMorseFreq() {
    return morse_freq;
}

// Set current morse code tone frequency (in Hz)

void HamShield::setMorseFreq(unsigned int morse_freq_hz) {
    morse_freq = morse_freq_hz;
}

// Get current duration of a morse dot (shorter is more WPM)

unsigned int HamShield::getMorseDotMillis() {
    return morse_dot_millis;
}

// Set current duration of a morse dot (shorter is more WPM)

void HamShield::setMorseDotMillis(unsigned int morse_dot_dur_millis) {
    morse_dot_millis = morse_dot_dur_millis;
}

/* Morse code out, blocking */

void HamShield::morseOut(char buffer[HAMSHIELD_MORSE_BUFFER_SIZE]) { 
  int i;
  char prev = 0;
  for(i = 0; buffer[i] != '\0' && i < HAMSHIELD_MORSE_BUFFER_SIZE; prev = buffer[i], i++) {
    
    // On a space, delay 7 dots
    if(buffer[i] == ' ') {
      // We delay by 4 here, if we previously sent a symbol. Otherwise 7.
      // This could probably just be always 7 and go relatively unnoticed.
      if(prev == 0 || prev == ' '){
        //tone(hs_mic_pin, 6000, morse_dot_millis * 7);
        HSnoTone(hs_mic_pin);
        HSdelay(morse_dot_millis*7);
        //Serial.print("  ");
      } else {
        //tone(hs_mic_pin, 6000, morse_dot_millis * 4);
        HSnoTone(hs_mic_pin);
        HSdelay(morse_dot_millis*4);
        //Serial.print(" ");
      }
      continue;
    }
    // Otherwise, lookup our character symbol
    uint8_t bits = morseLookup(buffer[i]);
    if(bits) { // If it is a valid character...
      do {
        if(bits & 1) {
          HStone(hs_mic_pin, morse_freq);//, morse_dot_millis * 3);
          HSdelay(morse_dot_millis*3);
          HSnoTone(hs_mic_pin);
          //Serial.print('-');
        } else {
          HStone(hs_mic_pin, morse_freq);//, morse_dot_millis);
          HSdelay(morse_dot_millis);
          HSnoTone(hs_mic_pin);
          //Serial.print('.');
        }
        //tone(hs_mic_pin, 6000, morse_dot_millis);
        HSnoTone(hs_mic_pin);
		HSdelay(morse_dot_millis);
        bits >>= 1; // Shift into the next symbol
      } while(bits != 1); // Wait for 1 termination to be all we have left
    }
    // End of character
    //tone(hs_mic_pin, 6000, morse_dot_millis * 3);
    HSnoTone(hs_mic_pin);
	HSdelay(morse_dot_millis * 3);
  }
  return;
}

// returns '\0' if no valid morse char found yet
char HamShield::morseRxLoop() {
  static uint32_t last_tone_check = 0; // track how often we check for morse tones
  static uint32_t tone_in_progress; // track how long the current tone lasts
  static uint32_t space_in_progress; // track how long since the last tone
  static uint8_t rx_morse_char;
  static uint8_t rx_morse_bit;
  static bool bits_to_process;
    
  if (last_tone_check == 0) {  
    last_tone_check = millis();
    space_in_progress = 0; // haven't checked yet
    tone_in_progress = 0; // not currently listening to a tone
    rx_morse_char = 0; // haven't found any tones yet
    rx_morse_bit = 1;
    bits_to_process = false;
  }
  
  char m = 0;
  
  // are we receiving anything
  if (toneDetected()) {
    space_in_progress = 0;
    if (tone_in_progress == 0) {
      // start a new tone
      tone_in_progress = millis();
      //Serial.print('t');
    }
  } else {
    // keep track of how long the silence is
    if (space_in_progress == 0) space_in_progress = millis();

    // we wait for a bit of silence before ending the last
    // symbol in order to smooth out the detector
    if ((millis() - space_in_progress) > SYMBOL_END_TIME)
    {
      if (tone_in_progress != 0) {
        // end the last tone
        uint16_t tone_time = millis() - tone_in_progress;
        tone_in_progress = 0;
        //Serial.println(tone_time);
        bits_to_process = handleMorseTone(tone_time, bits_to_process, &rx_morse_char, &rx_morse_bit);
      } 
    } 

    // we might be done with a character if the space is long enough
    if (((millis() - space_in_progress) > CHAR_END_TIME) && bits_to_process) {
      m = parseMorse(rx_morse_char, rx_morse_bit);
      bits_to_process = false;
      rx_morse_char = 0;
      rx_morse_bit = 1;
    }

    // we might be done with a message if the space is long enough
    if ((millis() - space_in_progress) > MESSAGE_END_TIME) {
      rx_morse_char = 0;
      rx_morse_bit = 1;
    }
  }
  
  return m;
}

bool HamShield::handleMorseTone(uint16_t tone_time, bool bits_to_process, 
                                uint8_t * rx_morse_char, uint8_t * rx_morse_bit) {  
  //Serial.println(tone_time);
  if (tone_time > MIN_DOT_TIME && tone_time < MAX_DOT_TIME) {
    // add a dot
    //Serial.print(".");
    bits_to_process = true;
    //nothing to do for this bit position, since . = 0
  } else if (tone_time > MIN_DASH_TIME && tone_time < MAX_DASH_TIME) {
    // add a dash
    //Serial.print("-");
    bits_to_process = true;
    *rx_morse_char += *rx_morse_bit;
  }

  // prep for the next bit
  *rx_morse_bit = *rx_morse_bit << 1;
  
  return bits_to_process;
}

char HamShield::parseMorse(uint8_t rx_morse_char, uint8_t rx_morse_bit) {
  // if morse_char is a valid morse character, return the character
  // if morse_char is an invalid (incomplete) morse character, return 0


  //if (rx_morse_bit != 1) Serial.println(rx_morse_char, BIN);
  rx_morse_char += rx_morse_bit; // add the terminator bit
  // if we got a char, then print it
  char c = morseReverseLookup(rx_morse_char);

  return c;
}


/* Morse code lookup table */

uint8_t HamShield::morseLookup(char letter) { 
  uint8_t i;
  for(i = 0; i < MORSE_TABLE_LENGTH; i++) {
#ifndef MORSE_TABLE_PROGMEM
    if(asciiMorse[i].ascii == letter)
      return asciiMorse[i].itu;
#else
    uint16_t w = pgm_read_word_near(asciiMorseProgmem + i);
    if( (char)((w>>8) & 0xff) == letter )
      return (uint8_t)(w & 0xff);
#endif // MORSE_TABLE_PROGMEM
  }
  return 0;
}

uint8_t HamShield::morseReverseLookup(uint8_t itu) {
  uint8_t i;
  for(i = 0; i < MORSE_TABLE_LENGTH; i++) {
#ifndef MORSE_TABLE_PROGMEM
    if(asciiMorse[i].itu == itu)
      return asciiMorse[i].ascii;
#else
    uint16_t w = pgm_read_word_near(asciiMorseProgmem + i);
    if( (uint8_t)(w & 0xff) == itu )
      return (char)((w>>8) & 0xff);
#endif // MORSE_TABLE_PROGMEM
  }
  return 0;
}

/* 

SSTV VIS Digital Header 

Reference: http://www.barberdsp.com/files/Dayton%20Paper.pdf

Millis    Freq    Description
-----------------------------------------------
300     1900     Leader tone
10     1200     break
300     1900     Leader tone
30     1200     VIS start bit
30     bit 0    1100hz = “1”, 1300hz = “0”
30     bit 1    “”
30     bit 2     “”
30    bit 3     “”
30     bit 4     “”
30     bit 5     “”
30     bit 6     “”
30    PARITY    Even=1300hz,Odd=1100hz
30    1200    VIS stop bit

*/

void HamShield::SSTVVISCode(int code) { 
    toneWait(1900,300);
    toneWait(1200,10);
    toneWait(1900,300);
    toneWait(1200,30);
        for(int x = 0; x < 7; x++) { 
           if(code&(1<<x)) { toneWait(1100,30); } else { toneWait(1300,30); } 
        } 
        if(parityCalc(code)) { toneWait(1300,30); } else { toneWait(1100,30); } 
        toneWait(1200,30);
        return;
}

/* 

SSTV Test Pattern 
Print 6 color bars
MARTIN1 is only supported for this
Reference: http://www.barberdsp.com/files/Dayton%20Paper.pdf

*/

void HamShield::SSTVTestPattern(int code) { 
       SSTVVISCode(code);
       if(code == MARTIN1) { 
            for(int x = 0; x < 257; x++){ 

                toneWaitU(1200,4862);              // sync pulse (4862 uS)
                toneWaitU(1500,572);               // sync porch (572 uS)

                /* Green Channel - 146.432ms a line (we are doing 144ms) */
 
                toneWait(2400,24);
                toneWait(2400,24);
                toneWait(2400,24);
                toneWait(2400,24);
                toneWait(1500,24);
                toneWait(1500,24); 

                toneWaitU(1500,572);               // color separator pulse (572 uS)

                /* Blue Channel - 146.432ms a line (we are doing 144ms) */
 
                toneWait(2400,24);
                toneWait(1500,24);
                toneWait(2400,24);
                toneWait(1500,24);
                toneWait(1500,24);
                toneWait(2400,24);  

                toneWaitU(1500,572);               // color separator pulse (572 uS)

                /* Red Channel - 146.432ms a line (we are doing 144ms) */

                toneWait(2400,24);
                toneWait(2400,24);
                toneWait(1500,24);
                toneWait(1500,24);
                toneWait(2400,24);
                toneWait(1500,24); 
 
                toneWaitU(1500,572);               // color separator pulse (572 uS)
             }
     }
}

/* wait for tone to complete */

void HamShield::toneWait(uint16_t freq, long timer) { 
    HStone(hs_mic_pin,freq);//,timer);
    HSdelay(timer);
    HSnoTone(hs_mic_pin);
}

/* wait microseconds for tone to complete */

void HamShield::toneWaitU(uint16_t freq, long timer) { 
    if(freq < 16383) { 
    HStone(hs_mic_pin,freq);
    HSdelayMicroseconds(timer); HSnoTone(hs_mic_pin); return;
    }
    HStone(hs_mic_pin,freq);
    HSdelay(timer / 1000); HSnoTone(hs_mic_pin); return;
}


bool HamShield::parityCalc(int code) {  
     unsigned int v;       // word value to compute the parity of
     bool parity = false;  // parity will be the parity of v

    while (code)
    {
       parity = !parity;
       code = code & (code - 1);
    }

    return parity;
}
