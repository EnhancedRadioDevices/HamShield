// HAMShield library collection
// Based on Programming Manual rev. 2.0, 5/19/2011 (RM-MPU-6000A-00)
// 11/22/2013 by Morgan Redfield <redfieldm@gmail.com>
// 04/26/2015 various changes Casey Halverson <spaceneedle@gmail.com>


#include "HAMShield.h"
#include <avr/wdt.h>
// #include <PCM.h>

/* don't change this regulatory value, use dangerMode() and safeMode() instead */

bool restrictions = true; 

/* channel lookup tables */

uint32_t FRS[] = {0,462562,462587,462612,462637,462662,462687,462712,467562,467587,467612,467637,467662,467687,467712};

uint32_t GMRS[] = {0,462550,462575,462600,462625,462650,462675,462700,462725};

uint32_t MURS[] = {0,151820,151880,151940,154570,154600};

uint32_t WX[] = {0,162550,162400,162475,162425,162450,162500,162525};

/* morse code lookup table */

const char *ascii = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?'!/()&:;=+-_\"$@",
  *itu[] = { ".-","-...","-.-.","-..",".","..-.","--.","....","..",".---","-.-",".-..","--","-.","---",".--.","--.-",".-.","...","-","..-","...-",".--","-..-","-.--","--..","-----",".----","..---","...--","....-",".....","-....","--...","---..","----.",".-.-.-","--..--","..--..",".----.","-.-.--","-..-.","-.--.","-.--.-",".-...","---...","-.-.-.","-...-",".-.-.","-....-","..--.-",".-..-.","...-..-",".--.-."
  };

/* 2200 Hz */

const unsigned char AFSK_mark[] PROGMEM = { 154, 249, 91, 11, 205, 216, 25, 68, 251, 146, 0, 147, 250, 68, 24, 218, 203, 13, 88, 254, 128, 1, 167, 242, 52, 37, 231, 186, 5, 108, 255, 108, 5, 186, 231, 37, 52, 242, 167, 1, 128, 254, 88, 13, 203, 218, 24, 69, 250, 147, 0, 147, 250, 69, 24, 218, 203, 13, 88, 255, 127, 2, 165, 245, 48 };

/* 1200 Hz */

const unsigned char AFSK_space[] PROGMEM = { 140, 228, 250, 166, 53, 0, 53, 166, 249, 230, 128, 24, 7, 88, 203, 255, 203, 88, 7, 24, 128, 230, 249, 167, 53, 0, 53, 167, 249, 230, 128, 24, 6, 88, 202, 255, 202, 88, 6, 24, 127, 231, 249, 167, 52, 0, 52, 167, 248, 231, 127, 25, 6, 89, 202, 255, 202, 89, 6, 25, 127, 231, 248, 167, 53, 0, 54, 165, 251, 227, 133, 14}; 


/* Aux button variables */

volatile int ptt = false;
volatile long bouncer = 0;

/** Default constructor, uses default I2C address.
 * @see A1846S_DEFAULT_ADDRESS
 */
HAMShield::HAMShield() {
    devAddr = A1846S_DEV_ADDR_SENLOW;
}

/** Specific address constructor.
 * @param address I2C address
 * @see A1846S_DEFAULT_ADDRESS
 * @see A1846S_ADDRESS_AD0_LOW
 * @see A1846S_ADDRESS_AD0_HIGH
 */
HAMShield::HAMShield(uint8_t address) {
    devAddr = address;
}

/** Power on and prepare for general usage.
 * 
 */
void HAMShield::initialize() {
   // set up PWM output for RF power control - commenting out to get rid of terrible buzzing noise
   // pwr_control_pin = 9;  
  
  
   // Note: these initial settings are for UHF 12.5kHz channel
   // see the A1846S register table and initial settings for more info
   
   // TODO: update code to make it easier to change from VHF to UHF and 12.5kHz channel to 25kHz channel
    uint16_t tx_data;
  
    // reset all registers in A1846S
    softReset();
    // set pdn_reg bit in control register (0 or 1?) (now done in softReset)
    //I2Cdev::writeBitW(devAddr, A1846S_CTL_REG, A1846S_PWR_DWN_BIT, 1);
    tx_data = 0x0698;
    I2Cdev::writeWord(devAddr, 0x02, tx_data);  // why is this here? See A1846S register init table

    //set up clock to ues 12-14MHz
    setClkMode(1);
        
    // set up clock to use 12.8MHz crystal
    setXtalFreq(12800);
    // set up ADClk frequency to 6.4MHz
    setAdcClkFreq(6400);
    
    tx_data = 0xE000;
    I2Cdev::writeWord(devAddr, 0x24, tx_data);  // why is this here? See A1846S register init word doc    

    //could change GPIO voltage levels with writes to 0x08 and 0x09
    // see A1846S register init table 
    tx_data = 0x03AC;
    I2Cdev::writeWord(devAddr, 0x09, tx_data);  // why is this here? See A1846S register init word doc    

    // set PA_bias voltage to 1.68V (and do something else too? what's the 3 for?)
    tx_data = 0x0320;
    I2Cdev::writeWord(devAddr, 0x0A, tx_data);
       
    tx_data = 0x1A10;
    I2Cdev::writeWord(devAddr, 0x0B, tx_data);  // why is this here? See A1846S register init table    

    tx_data = 0x3E37;
    I2Cdev::writeWord(devAddr, 0x11, tx_data);  // why is this here? See A1846S register init table    

    // Automatic Gain Control stuff
    // AGC when band is UHF,0x32 = 0x627C;when band is VHF,0x32 = 0x62BC//
    tx_data = 0x627c; // this is uhf, for vhf set to 0x62bc
    I2Cdev::writeWord(devAddr, 0x32, tx_data);  // why is this here? See A1846S register init table
    tx_data = 0x0AF2;
    I2Cdev::writeWord(devAddr, 0x33, tx_data);  // why is this here? See A1846S register init table

    // why is this here? See A1846S register init word doc
    tx_data = 0x0F28; // this is uhf, for vhf set to 0x62bc
    I2Cdev::writeWord(devAddr, 0x3C, tx_data);  // why is this here? See A1846S register init table
    tx_data = 0x200B;
    I2Cdev::writeWord(devAddr, 0x3D, tx_data);  // why is this here? See A1846S register init table

    // Noise threshold settings
    tx_data = 0x1C2F; // see email from Iris
    I2Cdev::writeWord(devAddr, 0x47, tx_data);  // why is this here? See A1846S register init table

    // SNR LPF settings, sq settings
    tx_data = 0x293A;
    I2Cdev::writeWord(devAddr, 0x4e, tx_data);  // why is this here? See A1846S register init table

    // subaudio decode setting,sq_out_sel,noise threshold value db
    tx_data = 0x114A; // A1846S_SQ_OUT_SEL_REG is 0x54
    I2Cdev::writeWord(devAddr, A1846S_SQ_OUT_SEL_REG, tx_data);  // why is this here? See A1846S register init table
    
    // bandwide setting of filter when RSSI is high or low
    tx_data = 0x0652;
    I2Cdev::writeWord(devAddr, 0x56, tx_data);  // why is this here? See A1846S register init table

    tx_data = 0x062d;
    I2Cdev::writeWord(devAddr, 0x6e, tx_data);  // why is this here? See A1846S register init table

    // note, this is for 12.5kHz channel
    tx_data = 0x6C1E;
    I2Cdev::writeWord(devAddr, 0x71, tx_data);  // why is this here? See A1846S register init table

    // see A1846S register init doc for this
    tx_data = 0x00FF;
    I2Cdev::writeWord(devAddr, 0x44, tx_data);  // why is this here? See A1846S register init table
    tx_data = 0x0500;
    I2Cdev::writeWord(devAddr, 0x1F, tx_data);  // set up GPIO for RX/TX mirroring


    // set RFoutput power (note that the address is 0x85, so do some rigmaroll)
    tx_data = 0x1;
    I2Cdev::writeWord(devAddr, 0x7F, tx_data);  // prep to write to a reg > 0x7F
    // If 0x85 is  0x001F, Rfoutput power is 8dBm , ACP is -63dB in 12.5KHz and -65dB in 25KHz
    // If 0x85 is  0x0018, Rfoutput power is 6dBm , ACP is -64dB in 12.5KHz and -66dB in 25KHz
    // If 0x85 is  0x0017, Rfoutput power is -3dBm , ACP is -68dBc in 12.5KHz and -68dBc in 25KHz
    tx_data = 0x001F;
    I2Cdev::writeWord(devAddr, 0x5, tx_data);  // set output power, reg 0x85 - 0x80
    tx_data = 0x0;
    I2Cdev::writeWord(devAddr, 0x7F, tx_data);  // finish writing to a reg > 0x7F
    
    // set control reg for pdn_reg, rx, and mute when rxno
    tx_data = 0xA4;
    I2Cdev::writeWord(devAddr, A1846S_CTL_REG, tx_data);  // finish writing to a reg > 0x7F
    
    delay(100);
    
    // set control reg for chip_cal_en, pdn_reg, rx, and mute when rxno
    tx_data = 0xA6;
    I2Cdev::writeWord(devAddr, A1846S_CTL_REG, tx_data);  // finish writing to a reg > 0x7F
    
    delay(100);

    // set control reg for chip_cal_en, pdn_reg
    tx_data = 0x6;
    I2Cdev::writeWord(devAddr, A1846S_CTL_REG, tx_data);  // finish writing to a reg > 0x7F
    
    delay(100);
    
    // and then I have no idea about this nonsense
    // some of these settings seem to be for 12.5kHz channels
    // TODO: get A1846S to give us a full register table
    tx_data = 0x1d40;
    I2Cdev::writeWord(devAddr, 0x54, tx_data);
    tx_data = 0x062d;
    I2Cdev::writeWord(devAddr, 0x6e, tx_data);
    tx_data = 0x102a;
    I2Cdev::writeWord(devAddr, 0x70, tx_data);
    tx_data = 0x6c1e;
    I2Cdev::writeWord(devAddr, 0x71, tx_data);
    tx_data = 0x0006;
    I2Cdev::writeWord(devAddr, 0x30, tx_data);
    
    delay(100);

    // setup default values

    setFrequency(446000);
    setVolume1(0xF);
    setVolume2(0xF);
    setModeReceive();
    setTxSourceMic();
    setSQLoThresh(80);
    setSQOn();

}

/** Verify the I2C connection.
 * Make sure the device is connected and responds as expected.
 * @return True if connection is valid, false otherwise
 */
bool HAMShield::testConnection() {
    I2Cdev::readWord(devAddr, 0x09, radio_i2c_buf);
// 03ac or 032c
    return radio_i2c_buf[0] == 0x03AC; // TODO: find a device ID reg I can use
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
 *   Move 7FH 0x0001{

}
 *   Move 05H 0x001F{

} 05H=85H-80H
 *   Move 7FH 0x0000{

}
 */

uint16_t HAMShield::readCtlReg() {
  I2Cdev::readWord(devAddr, A1846S_CTL_REG, radio_i2c_buf);
  return radio_i2c_buf[0];
}

void HAMShield::softReset() {
   uint16_t tx_data = 0x1;
   I2Cdev::writeWord(devAddr, A1846S_CTL_REG, tx_data);
   delay(100); // Note: see A1846S setup info for timing guidelines
   tx_data = 0x4;
   I2Cdev::writeWord(devAddr, A1846S_CTL_REG, tx_data);
}

 
void HAMShield::setFrequency(uint32_t freq_khz) {
    radio_frequency = freq_khz;
    uint32_t freq_raw = freq_khz << 3; // shift by 3 to multiply by 8

    // send top 16 bits to A1846S_FREQ_HI_REG	
    uint16_t freq_half = (uint16_t) (0x3FFF & (freq_raw >> 16));
    I2Cdev::writeWord(devAddr, A1846S_FREQ_HI_REG, freq_half);
    //  send bottom 16 bits to A1846S_FREQ_LO_REG
    freq_half = (uint16_t) (freq_raw & 0xFFFF);
    I2Cdev::writeWord(devAddr, A1846S_FREQ_LO_REG, freq_half);
}

uint32_t HAMShield::getFrequency() {
  return radio_frequency;
}

void HAMShield::setUHF() {
  setGpioHi(2); // turn off VHF
  setGpioLow(3); // turn on UHF
}

void HAMShield::setVHF() {
  setGpioHi(3); // turn off UHF
  setGpioLow(2); // turn on VHF
}

void HAMShield::setNoFilters() {
  setGpioHi(3); // turn off UHF
  setGpioHi(2); // turn off VHF
}

// band
// 00 - 400-520MHz 
// 10 - 200-260MHz
// 11 - 134-174MHz
// TODO: add write to 0x32 based on band selection
void HAMShield::setBand(uint16_t band){
    if (band == 0) {
      setUHF();
    } else if (band == 2) {
      // not quite in the band for our filters, but use VHF
      setVHF();
    } else if (band == 3) {
      setVHF();
    } else {
      // illegal write code, turn UHF and VHF channels both off
      setNoFilters();
      // turn off transmit as well to make sure we don't break anything
      setTX(0);
    }
    I2Cdev::writeBitsW(devAddr, A1846S_BAND_SEL_REG, A1846S_BAND_SEL_BIT, A1846S_BAND_SEL_LENGTH, band);
}
uint16_t HAMShield::getBand(){
    I2Cdev::readBitsW(devAddr, A1846S_BAND_SEL_REG, A1846S_BAND_SEL_BIT, A1846S_BAND_SEL_LENGTH, radio_i2c_buf);
    return radio_i2c_buf[0];
}

// xtal frequency (kHz)
// 12-14MHz crystal: this reg is set to crystal freq_khz
// 24-28MHz crystal: this reg is set to crystal freq_khz / 2
void HAMShield::setXtalFreq(uint16_t freq_kHz){
	I2Cdev::writeWord(devAddr, A1846S_XTAL_FREQ_REG, freq_kHz);	
}
uint16_t HAMShield::getXtalFreq(){
	I2Cdev::readWord(devAddr, A1846S_FREQ_HI_REG, radio_i2c_buf);
	
	return radio_i2c_buf[0];
}

// adclk frequency (kHz)
// 12-14MHz crystal: this reg is set to crystal freq_khz / 2
// 24-28MHz crystal: this reg is set to crystal freq_khz / 4
void HAMShield::setAdcClkFreq(uint16_t freq_kHz){
	I2Cdev::writeWord(devAddr, A1846S_ADCLK_FREQ_REG, freq_kHz);	
}

uint16_t HAMShield::getAdcClkFreq(){
	I2Cdev::readWord(devAddr, A1846S_ADCLK_FREQ_REG, radio_i2c_buf);
	return radio_i2c_buf[0];
}

// clk mode
// 12-14MHz: set to 1
// 24-28MHz: set to 0
void HAMShield::setClkMode(bool LFClk){
    // include upper bits as default values
    uint16_t tx_data = 0x0F11; // NOTE: should this be 0fd1 or 0f11? Programming guide and setup guide disagree
    if (!LFClk) {
      tx_data = 0x0F10;  
    }
  
    I2Cdev::writeWord(devAddr, A1846S_CLK_MODE_REG, tx_data);
}
bool HAMShield::getClkMode(){
    I2Cdev::readBitW(devAddr, A1846S_CLK_MODE_REG, A1846S_CLK_MODE_BIT, radio_i2c_buf);
    return (radio_i2c_buf[0] != 0);
}

// clk example
// 12.8MHz clock
// A1846S_XTAL_FREQ_REG[15:0]= xtal_freq<15:0>=12.8*1000=12800
// A1846S_ADCLK_FREQ_REG[12:0] =adclk_freq<15:0>=(12.8/2)*1000=6400
// A1846S_CLK_MODE_REG[0]= clk_mode =1

// TX/RX control

// channel mode
// 11 - 25kHz channel
// 00 - 12.5kHz channel
// 10,01 - reserved
void HAMShield::setChanMode(uint16_t mode){
    I2Cdev::writeBitsW(devAddr, A1846S_CTL_REG, A1846S_CHAN_MODE_BIT, A1846S_CHAN_MODE_LENGTH, mode);
}
uint16_t HAMShield::getChanMode(){
    I2Cdev::readBitsW(devAddr, A1846S_CTL_REG, A1846S_CHAN_MODE_BIT, A1846S_CHAN_MODE_LENGTH, radio_i2c_buf);
    return radio_i2c_buf[0];
}

// choose tx or rx
void HAMShield::setTX(bool on_noff){
    // make sure RX is off
    if (on_noff) {
      setRX(false);
      
      // For RF6886:
      // first turn on power
        // set RX output on
      setGpioHi(4); // remember that RX and TX are active low
        // set TX output off
      setGpioLow(5); // remember that RX and TX are active low
      // then turn on VREG (PWM output)
      // then apply RF signal
      setRfPower(100); // figure out a good default number (or don't set a default)
    }

    // todo: make sure gpio are set correctly after this
    I2Cdev::writeBitW(devAddr, A1846S_CTL_REG, A1846S_TX_MODE_BIT, on_noff);
    
    
}
bool HAMShield::getTX(){
    I2Cdev::readBitW(devAddr, A1846S_CTL_REG, A1846S_TX_MODE_BIT, radio_i2c_buf);
    return (radio_i2c_buf[0] != 0);
}

void HAMShield::setRX(bool on_noff){
   // make sure TX is off
   if (on_noff) {
     setTX(false);
     
    // set TX output off
    setGpioHi(5); // remember that RX and TX are active low
    // set RX output on
    setGpioLow(4); // remember that RX and TX are active low
   }
   
   I2Cdev::writeBitW(devAddr, A1846S_CTL_REG, A1846S_RX_MODE_BIT, on_noff);
}
bool HAMShield::getRX(){
    I2Cdev::readBitW(devAddr, A1846S_CTL_REG, A1846S_RX_MODE_BIT, radio_i2c_buf);
    return (radio_i2c_buf[0] != 0);
}

void HAMShield::setModeTransmit(){
        // check to see if we should allow them to do this
        if(restrictions == true) { 
           if((radio_frequency > 139999) & (radio_frequency < 148001)) { setRX(false); setTX(true); } 
           if((radio_frequency > 218999) & (radio_frequency < 225001)) { setRX(false); setTX(true); } 
           if((radio_frequency > 419999) & (radio_frequency < 450001)) { setRX(false); setTX(true); }                     
        } else { 
	// turn off rx, turn on tx
	setRX(false); // break before make
	setTX(true); }
} 
void HAMShield::setModeReceive(){
	// turn on rx, turn off tx
	setTX(false); // break before make
	setRX(true);
} 
void HAMShield::setModeOff(){
	// turn off rx, turn off tx, set pwr_dwn bit
	setTX(false);
	setRX(false);
} 

// set tx source
// 00 - Mic source
// 01 - sine source from tone2
// 10 - tx code from GPIO1 code_in (gpio1<1:0> must be set to 01)
// 11 - no tx source
void HAMShield::setTxSource(uint16_t tx_source){
    I2Cdev::writeBitsW(devAddr, A1846S_TX_VOICE_REG, A1846S_VOICE_SEL_BIT, A1846S_VOICE_SEL_LENGTH, tx_source);
}
void HAMShield::setTxSourceMic(){
	setTxSource(0);
}
void HAMShield::setTxSourceSine(){
	setTxSource(1);
}
void HAMShield::setTxSourceCode(){
	// note, also set GPIO1 to 01
	setGpioMode(1, 1);
	
	setTxSource(2);
}
void HAMShield::setTxSourceNone(){
	setTxSource(3);
}
uint16_t HAMShield::getTxSource(){
    I2Cdev::readBitsW(devAddr, A1846S_TX_VOICE_REG, A1846S_VOICE_SEL_BIT, A1846S_VOICE_SEL_LENGTH, radio_i2c_buf);
    return radio_i2c_buf[0];
}

// set PA_bias voltage
//    000000: 1.01V
//    000001:1.05V
//    000010:1.09V
//    000100: 1.18V
//    001000: 1.34V
//    010000: 1.68V
//    100000: 2.45V
//    1111111:3.13V
void HAMShield::setPABiasVoltage(uint16_t voltage){
    I2Cdev::writeBitsW(devAddr, A1846S_PABIAS_REG, A1846S_PABIAS_BIT, A1846S_PABIAS_LENGTH, voltage);
}
uint16_t HAMShield::getPABiasVoltage(){
    I2Cdev::readBitsW(devAddr, A1846S_PABIAS_REG, A1846S_PABIAS_BIT, A1846S_PABIAS_LENGTH, radio_i2c_buf);
    return radio_i2c_buf[0];
}

// Subaudio settings
// TX and RX code
/*
	Set code mode:
	Step1: set 58H[1:0]=11 set voice hpf bypass
	Step2: set 58H[5:3]=111 set voice lpf bypass and pre/de-emph bypass
	Step3 set 3CH[15:14]=10 set code mode
	Step4: set 1FH[3:2]=01 set GPIO code in or code out

	TX code mode:
	Step1: 45H[2:0]=010
	
	RX code mode:
	Step1: set 45H[2:0]=001
	Step2: set 4dH[15:10]=000001
*/

//   Ctcss/cdcss mode sel
//      x00=disable,
//      001=inner ctcss en,
//      010= inner cdcss en
//      101= outer ctcss en,
//      110=outer cdcss en
//      others =disable
void HAMShield::setCtcssCdcssMode(uint16_t mode){
    I2Cdev::writeBitsW(devAddr, A1846S_SUBAUDIO_REG, A1846S_C_MODE_BIT, A1846S_C_MODE_LENGTH, mode);
}
uint16_t HAMShield::getCtcssCdcssMode(){
    I2Cdev::readBitsW(devAddr, A1846S_SUBAUDIO_REG, A1846S_C_MODE_BIT, A1846S_C_MODE_LENGTH, radio_i2c_buf);
    return radio_i2c_buf[0];
}
void HAMShield::setInnerCtcssMode(){
	setCtcssCdcssMode(1);
}
void HAMShield::setInnerCdcssMode(){
	setCtcssCdcssMode(2);
}
void HAMShield::setOuterCtcssMode(){
	setCtcssCdcssMode(5);
}
void HAMShield::setOuterCdcssMode(){
	setCtcssCdcssMode(6);
}
void HAMShield::disableCtcssCdcss(){
	setCtcssCdcssMode(0);
}

//   Ctcss_sel
//      1 = ctcss_cmp/cdcss_cmp out via gpio
//      0 = ctcss/cdcss sdo out vio gpio
void HAMShield::setCtcssSel(bool cmp_nsdo){
    I2Cdev::writeBitW(devAddr, A1846S_SUBAUDIO_REG, A1846S_CTCSS_SEL_BIT, cmp_nsdo);
}
bool HAMShield::getCtcssSel(){
    I2Cdev::readBitW(devAddr, A1846S_SUBAUDIO_REG, A1846S_CTCSS_SEL_BIT, radio_i2c_buf);
    return (radio_i2c_buf[0] != 0);
}

//   Cdcss_sel
//      1 = long (24 bit) code
//      0 = short(23 bit) code
void HAMShield::setCdcssSel(bool long_nshort){
    I2Cdev::writeBitW(devAddr, A1846S_SUBAUDIO_REG, A1846S_CDCSS_SEL_BIT, long_nshort);
}
bool HAMShield::getCdcssSel(){
    I2Cdev::readBitW(devAddr, A1846S_SUBAUDIO_REG, A1846S_CDCSS_SEL_BIT, radio_i2c_buf);
    return (radio_i2c_buf[0] != 0);
}

// Cdcss neg_det_en
void HAMShield::enableCdcssNegDet(){
    I2Cdev::writeBitW(devAddr, A1846S_SUBAUDIO_REG, A1846S_NEG_DET_EN_BIT, 1);
}
void HAMShield::disableCdcssNegDet(){
    I2Cdev::writeBitW(devAddr, A1846S_SUBAUDIO_REG, A1846S_NEG_DET_EN_BIT, 0);
}
bool HAMShield::getCdcssNegDetEnabled(){
    I2Cdev::readBitW(devAddr, A1846S_SUBAUDIO_REG, A1846S_NEG_DET_EN_BIT, radio_i2c_buf);
    return (radio_i2c_buf[0] != 0);
}

// Cdcss pos_det_en
void HAMShield::enableCdcssPosDet(){
    I2Cdev::writeBitW(devAddr, A1846S_SUBAUDIO_REG, A1846S_POS_DET_EN_BIT, 1);
}
void HAMShield::disableCdcssPosDet(){
    I2Cdev::writeBitW(devAddr, A1846S_SUBAUDIO_REG, A1846S_POS_DET_EN_BIT, 0);
}
bool HAMShield::getCdcssPosDetEnabled(){
    I2Cdev::readBitW(devAddr, A1846S_SUBAUDIO_REG, A1846S_POS_DET_EN_BIT, radio_i2c_buf);
    return (radio_i2c_buf[0] != 0);
}

// css_det_en
void HAMShield::enableCssDet(){
    I2Cdev::writeBitW(devAddr, A1846S_SUBAUDIO_REG, A1846S_CSS_DET_EN_BIT, 1);
}
void HAMShield::disableCssDet(){
    I2Cdev::writeBitW(devAddr, A1846S_SUBAUDIO_REG, A1846S_CSS_DET_EN_BIT, 0);
}
bool HAMShield::getCssDetEnabled(){
    I2Cdev::readBitW(devAddr, A1846S_SUBAUDIO_REG, A1846S_CSS_DET_EN_BIT, radio_i2c_buf);
    return (radio_i2c_buf[0] != 0);
}

// ctcss freq
void HAMShield::setCtcss(float freq) {
        int dfreq = freq / 10000;
        dfreq = dfreq * 65536;
        setCtcssFreq(dfreq);
}

void HAMShield::setCtcssFreq(uint16_t freq){
	I2Cdev::writeWord(devAddr, A1846S_CTCSS_FREQ_REG, freq);
}
uint16_t HAMShield::getCtcssFreq(){
	I2Cdev::readWord(devAddr, A1846S_CTCSS_FREQ_REG, radio_i2c_buf);
	
	return radio_i2c_buf[0];
}
void HAMShield::setCtcssFreqToStandard(){
	// freq must be 134.4Hz for standard cdcss mode
	setCtcssFreq(0x2268);
} 

// cdcss codes
void HAMShield::setCdcssCode(uint16_t code) {
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
	I2Cdev::writeWord(devAddr, A1846S_CDCSS_CODE_HI_REG, temp_code);
        temp_code = (uint16_t) (cdcss_code >> 16);	
	I2Cdev::writeWord(devAddr, A1846S_CDCSS_CODE_LO_REG, temp_code);	
}
uint16_t HAMShield::getCdcssCode() {
	uint32_t oct_code;
	I2Cdev::readWord(devAddr, A1846S_CDCSS_CODE_HI_REG, radio_i2c_buf);
	oct_code = (radio_i2c_buf[0] << 16);
	I2Cdev::readWord(devAddr, A1846S_CDCSS_CODE_LO_REG, radio_i2c_buf);
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
void HAMShield::setSQOn(){
    I2Cdev::writeBitW(devAddr, A1846S_CTL_REG, A1846S_SQ_ON_BIT, 1);
}
void HAMShield::setSQOff(){
    I2Cdev::writeBitW(devAddr, A1846S_CTL_REG, A1846S_SQ_ON_BIT, 0);
}
bool HAMShield::getSQState(){
    I2Cdev::readBitW(devAddr, A1846S_CTL_REG, A1846S_SQ_ON_BIT, radio_i2c_buf);
    return (radio_i2c_buf[0] != 0);
}

// SQ threshold
void HAMShield::setSQHiThresh(uint16_t sq_hi_threshold){
	// Sq detect high th, rssi_cmp will be 1 when rssi>th_h_sq, unit 1/8dB
	I2Cdev::writeWord(devAddr, A1846S_SQ_OPEN_THRESH_REG, sq_hi_threshold);
} 
uint16_t HAMShield::getSQHiThresh(){
	I2Cdev::readWord(devAddr, A1846S_SQ_OPEN_THRESH_REG, radio_i2c_buf);
	
	return radio_i2c_buf[0];
}
void HAMShield::setSQLoThresh(uint16_t sq_lo_threshold){
	// Sq detect low th, rssi_cmp will be 0 when rssi<th_l_sq && time delay meet, unit 1/8 dB
	I2Cdev::writeWord(devAddr, A1846S_SQ_SHUT_THRESH_REG, sq_lo_threshold);
}
uint16_t HAMShield::getSQLoThresh(){
	I2Cdev::readWord(devAddr, A1846S_SQ_SHUT_THRESH_REG, radio_i2c_buf);
	
	return radio_i2c_buf[0];
}

// SQ out select
void HAMShield::setSQOutSel(){
    I2Cdev::writeBitW(devAddr, A1846S_SQ_OUT_SEL_REG, A1846S_SQ_OUT_SEL_BIT, 1);
}
void HAMShield::clearSQOutSel(){
    I2Cdev::writeBitW(devAddr, A1846S_SQ_OUT_SEL_REG, A1846S_SQ_OUT_SEL_BIT, 0);
}
bool HAMShield::getSQOutSel(){
    I2Cdev::readBitW(devAddr, A1846S_SQ_OUT_SEL_REG, A1846S_SQ_OUT_SEL_BIT, radio_i2c_buf);
    return (radio_i2c_buf[0] != 0);
}

// VOX
void HAMShield::setVoxOn(){
    I2Cdev::writeBitW(devAddr, A1846S_CTL_REG, A1846S_VOX_ON_BIT, 1);
}
void HAMShield::setVoxOff(){
    I2Cdev::writeBitW(devAddr, A1846S_CTL_REG, A1846S_VOX_ON_BIT, 0);
}
bool HAMShield::getVoxOn(){
    I2Cdev::readBitW(devAddr, A1846S_CTL_REG, A1846S_VOX_ON_BIT, radio_i2c_buf);
    return (radio_i2c_buf[0] != 0);
}

// Vox Threshold
void HAMShield::setVoxOpenThresh(uint16_t vox_open_thresh){
	// When vssi > th_h_vox, then vox will be 1(unit mV )
	I2Cdev::writeWord(devAddr, A1846S_TH_H_VOX_REG, vox_open_thresh);	
} 
uint16_t HAMShield::getVoxOpenThresh(){
	I2Cdev::readWord(devAddr, A1846S_TH_H_VOX_REG, radio_i2c_buf);
	
	return radio_i2c_buf[0];
}
void HAMShield::setVoxShutThresh(uint16_t vox_shut_thresh){
	// When vssi < th_l_vox && time delay meet, then vox will be 0 (unit mV )
	I2Cdev::writeWord(devAddr, A1846S_TH_L_VOX_REG, vox_shut_thresh);	
} 
uint16_t HAMShield::getVoxShutThresh(){
	I2Cdev::readWord(devAddr, A1846S_TH_L_VOX_REG, radio_i2c_buf);
	
	return radio_i2c_buf[0];
}

// Tail Noise
void HAMShield::enableTailNoiseElim(){
    I2Cdev::writeBitW(devAddr, A1846S_CTL_REG, A1846S_TAIL_ELIM_EN_BIT, 1);
}
void HAMShield::disableTailNoiseElim(){
    I2Cdev::writeBitW(devAddr, A1846S_CTL_REG, A1846S_TAIL_ELIM_EN_BIT, 1);
}
bool HAMShield::getTailNoiseElimEnabled(){
    I2Cdev::readBitW(devAddr, A1846S_CTL_REG, A1846S_TAIL_ELIM_EN_BIT, radio_i2c_buf);
    return (radio_i2c_buf[0] != 0);
}

// tail noise shift select
//   Select ctcss phase shift when use tail eliminating function when TX
//     00 = 120 degree shift
//     01 = 180 degree shift
//     10 = 240 degree shift
//     11 = reserved
void HAMShield::setShiftSelect(uint16_t shift_sel){
    I2Cdev::writeBitsW(devAddr, A1846S_SUBAUDIO_REG, A1846S_SHIFT_SEL_BIT, A1846S_SHIFT_SEL_LENGTH, shift_sel);
}
uint16_t HAMShield::getShiftSelect(){
    I2Cdev::readBitsW(devAddr, A1846S_SUBAUDIO_REG, A1846S_SHIFT_SEL_BIT, A1846S_SHIFT_SEL_LENGTH, radio_i2c_buf);
    return radio_i2c_buf[0];
}

// DTMF
void HAMShield::setDTMFC0(uint16_t freq) {
    I2Cdev::writeBitsW(devAddr, A1846S_DTMF_C01_REG, A1846S_DTMF_C0_BIT, A1846S_DTMF_C0_LENGTH, freq);
}
uint16_t HAMShield::getDTMFC0() {
    I2Cdev::readBitsW(devAddr, A1846S_DTMF_C01_REG, A1846S_DTMF_C0_BIT, A1846S_DTMF_C0_LENGTH, radio_i2c_buf);
    return radio_i2c_buf[0];
}
void HAMShield::setDTMFC1(uint16_t freq) {
    I2Cdev::writeBitsW(devAddr, A1846S_DTMF_C01_REG, A1846S_DTMF_C1_BIT, A1846S_DTMF_C1_LENGTH, freq);
}
uint16_t HAMShield::getDTMFC1()	 {
    I2Cdev::readBitsW(devAddr, A1846S_DTMF_C01_REG, A1846S_DTMF_C1_BIT, A1846S_DTMF_C1_LENGTH, radio_i2c_buf);
    return radio_i2c_buf[0];
}
void HAMShield::setDTMFC2(uint16_t freq) {
    I2Cdev::writeBitsW(devAddr, A1846S_DTMF_C23_REG, A1846S_DTMF_C2_BIT, A1846S_DTMF_C2_LENGTH, freq);
}
uint16_t HAMShield::getDTMFC2() {
    I2Cdev::readBitsW(devAddr, A1846S_DTMF_C23_REG, A1846S_DTMF_C2_BIT, A1846S_DTMF_C2_LENGTH, radio_i2c_buf);
    return radio_i2c_buf[0];
}
void HAMShield::setDTMFC3(uint16_t freq) {
    I2Cdev::writeBitsW(devAddr, A1846S_DTMF_C23_REG, A1846S_DTMF_C3_BIT, A1846S_DTMF_C3_LENGTH, freq);
}
uint16_t HAMShield::getDTMFC3() {
    I2Cdev::readBitsW(devAddr, A1846S_DTMF_C23_REG, A1846S_DTMF_C3_BIT, A1846S_DTMF_C3_LENGTH, radio_i2c_buf);
    return radio_i2c_buf[0];
}
void HAMShield::setDTMFC4(uint16_t freq) {
    I2Cdev::writeBitsW(devAddr, A1846S_DTMF_C45_REG, A1846S_DTMF_C4_BIT, A1846S_DTMF_C4_LENGTH, freq);
}
uint16_t HAMShield::getDTMFC4() {
    I2Cdev::readBitsW(devAddr, A1846S_DTMF_C45_REG, A1846S_DTMF_C4_BIT, A1846S_DTMF_C4_LENGTH, radio_i2c_buf);
    return radio_i2c_buf[0];
}
void HAMShield::setDTMFC5(uint16_t freq) {
    I2Cdev::writeBitsW(devAddr, A1846S_DTMF_C45_REG, A1846S_DTMF_C5_BIT, A1846S_DTMF_C5_LENGTH, freq);
}
uint16_t HAMShield::getDTMFC5() {
    I2Cdev::readBitsW(devAddr, A1846S_DTMF_C45_REG, A1846S_DTMF_C5_BIT, A1846S_DTMF_C5_LENGTH, radio_i2c_buf);
    return radio_i2c_buf[0];
}
void HAMShield::setDTMFC6(uint16_t freq) {
    I2Cdev::writeBitsW(devAddr, A1846S_DTMF_C67_REG, A1846S_DTMF_C6_BIT, A1846S_DTMF_C6_LENGTH, freq);
}
uint16_t HAMShield::getDTMFC6() {
    I2Cdev::readBitsW(devAddr, A1846S_DTMF_C67_REG, A1846S_DTMF_C6_BIT, A1846S_DTMF_C6_LENGTH, radio_i2c_buf);
    return radio_i2c_buf[0];
}
void HAMShield::setDTMFC7(uint16_t freq) {
    I2Cdev::writeBitsW(devAddr, A1846S_DTMF_C67_REG, A1846S_DTMF_C7_BIT, A1846S_DTMF_C7_LENGTH, freq);
}
uint16_t HAMShield::getDTMFC7() {
    I2Cdev::readBitsW(devAddr, A1846S_DTMF_C67_REG, A1846S_DTMF_C7_BIT, A1846S_DTMF_C7_LENGTH, radio_i2c_buf);
    return radio_i2c_buf[0];
}

// TX FM deviation
void HAMShield::setFMVoiceCssDeviation(uint16_t deviation){
    I2Cdev::writeBitsW(devAddr, A1846S_FM_DEV_REG, A1846S_FM_DEV_VOICE_BIT, A1846S_FM_DEV_VOICE_LENGTH, deviation);
}
uint16_t HAMShield::getFMVoiceCssDeviation(){
    I2Cdev::readBitsW(devAddr, A1846S_FM_DEV_REG, A1846S_FM_DEV_VOICE_BIT, A1846S_FM_DEV_VOICE_LENGTH, radio_i2c_buf);
    return radio_i2c_buf[0];
}
void HAMShield::setFMCssDeviation(uint16_t deviation){
    I2Cdev::writeBitsW(devAddr, A1846S_FM_DEV_REG, A1846S_FM_DEV_CSS_BIT, A1846S_FM_DEV_CSS_LENGTH, deviation);
}
uint16_t HAMShield::getFMCssDeviation(){
    I2Cdev::readBitsW(devAddr, A1846S_FM_DEV_REG, A1846S_FM_DEV_CSS_BIT, A1846S_FM_DEV_CSS_LENGTH, radio_i2c_buf);
    return radio_i2c_buf[0];
}

// RX voice range
void HAMShield::setVolume1(uint16_t volume){
    I2Cdev::writeBitsW(devAddr, A1846S_RX_VOLUME_REG, A1846S_RX_VOL_1_BIT, A1846S_RX_VOL_1_LENGTH, volume);
}
uint16_t HAMShield::getVolume1(){
    I2Cdev::readBitsW(devAddr, A1846S_RX_VOLUME_REG, A1846S_RX_VOL_1_BIT, A1846S_RX_VOL_1_LENGTH, radio_i2c_buf);
    return radio_i2c_buf[0];
}
void HAMShield::setVolume2(uint16_t volume){
    I2Cdev::writeBitsW(devAddr, A1846S_RX_VOLUME_REG, A1846S_RX_VOL_2_BIT, A1846S_RX_VOL_2_LENGTH, volume);
}
uint16_t HAMShield::getVolume2(){
    I2Cdev::readBitsW(devAddr, A1846S_RX_VOLUME_REG, A1846S_RX_VOL_2_BIT, A1846S_RX_VOL_2_LENGTH, radio_i2c_buf);
    return radio_i2c_buf[0];
}

// GPIO
void HAMShield::setGpioMode(uint16_t gpio, uint16_t mode){
	uint16_t mode_len = 2;
	uint16_t bit = gpio*2 + 1;

    I2Cdev::writeBitsW(devAddr, A1846S_GPIO_MODE_REG, bit, mode_len, mode);
}
void HAMShield::setGpioHiZ(uint16_t gpio){
	setGpioMode(gpio, 0);
}
void HAMShield::setGpioFcn(uint16_t gpio){
	setGpioMode(gpio, 1);
}
void HAMShield::setGpioLow(uint16_t gpio){
	setGpioMode(gpio, 2);
}
void HAMShield::setGpioHi(uint16_t gpio){
	setGpioMode(gpio, 3);
}
uint16_t HAMShield::getGpioMode(uint16_t gpio){
	uint16_t mode_len = 2;
	uint16_t bit = gpio*2 + 1;
	
    I2Cdev::readBitsW(devAddr, A1846S_GPIO_MODE_REG, bit, mode_len, radio_i2c_buf);
    return radio_i2c_buf[0];
}

// Int
void HAMShield::enableInterrupt(uint16_t interrupt){
    I2Cdev::writeBitW(devAddr, A1846S_INT_MODE_REG, interrupt, 1);
}
void HAMShield::disableInterrupt(uint16_t interrupt){
    I2Cdev::writeBitW(devAddr, A1846S_INT_MODE_REG, interrupt, 0);
}
bool HAMShield::getInterruptEnabled(uint16_t interrupt){
    I2Cdev::readBitW(devAddr, A1846S_INT_MODE_REG, interrupt, radio_i2c_buf);
    return (radio_i2c_buf[0] != 0);
}

// ST mode
void HAMShield::setStMode(uint16_t mode){
    I2Cdev::writeBitsW(devAddr, A1846S_CTL_REG, A1846S_ST_MODE_BIT, A1846S_ST_MODE_LENGTH, mode);
}
uint16_t HAMShield::getStMode(){
    I2Cdev::readBitsW(devAddr, A1846S_CTL_REG, A1846S_ST_MODE_BIT, A1846S_ST_MODE_LENGTH, radio_i2c_buf);
    return radio_i2c_buf[0];
}
void HAMShield::setStFullAuto(){
setStMode(2);
}
void HAMShield::setStRxAutoTxManu(){
setStMode(1);
}
void HAMShield::setStFullManu(){
setStMode(0);
}

// Pre-emphasis, De-emphasis filter
void HAMShield::bypassPreDeEmph(){
    I2Cdev::writeBitW(devAddr, A1846S_EMPH_FILTER_REG, A1846S_EMPH_FILTER_EN, 1);
}
void HAMShield::usePreDeEmph(){
    I2Cdev::writeBitW(devAddr, A1846S_EMPH_FILTER_REG, A1846S_EMPH_FILTER_EN, 0);
}
bool HAMShield::getPreDeEmphEnabled(){
    I2Cdev::readBitW(devAddr, A1846S_EMPH_FILTER_REG, A1846S_EMPH_FILTER_EN, radio_i2c_buf);
    return (radio_i2c_buf[0] != 0);
}

// Read Only Status Registers
int16_t HAMShield::readRSSI(){
	I2Cdev::readWord(devAddr, A1846S_RSSI_REG, radio_i2c_buf);
	
        int16_t rssi = (radio_i2c_buf[0] & 0x3FF) / 8 - 135;
	return rssi; // only need lowest 10 bits
}
uint16_t HAMShield::readVSSI(){
	I2Cdev::readWord(devAddr, A1846S_VSSI_REG, radio_i2c_buf);
	
	return radio_i2c_buf[0] & 0x7FF; // only need lowest 10 bits
}
uint16_t HAMShield::readDTMFIndex(){
// TODO: may want to split this into two (index1 and index2)
    I2Cdev::readBitsW(devAddr, A1846S_DTMF_RX_REG, A1846S_DTMF_INDEX_BIT, A1846S_DTMF_INDEX_LENGTH, radio_i2c_buf);
    return radio_i2c_buf[0];
} 
uint16_t HAMShield::readDTMFCode(){
//  1:f0+f4, 2:f0+f5, 3:f0+f6, A:f0+f7,
//  4:f1+f4, 5:f1+f5, 6:f1+f6, B:f1+f7,
//  7:f2+f4, 8:f2+f5, 9:f2+f6, C:f2+f7,
//  E(*):f3+f4, 0:f3+f5, F(#):f3+f6, D:f3+f7
    I2Cdev::readBitsW(devAddr, A1846S_DTMF_RX_REG, A1846S_DTMF_CODE_BIT, A1846S_DTMF_CODE_LENGTH, radio_i2c_buf);
    return radio_i2c_buf[0];
}

void HAMShield::setRfPower(uint8_t pwr) {
  
   // using loop reference voltage input to op-amp
   // (see RF6886 datasheet)
   // 30 is 0.5V, which is ~min loop reference voltage
   // 127 is 2.5V, which is ~max loop ref voltage
   int max_pwr = 255; //167; // 167 is 3.3*255/5 - 1;
   if (pwr > max_pwr) {
     pwr = max_pwr; 
   }
  
  
   // using open loop reference voltage into Vreg1/2
   /*int max_pwr = 78; // 78 = 1.58*255/5 - 1
   if (pwr > max_pwr) {
     pwr = max_pwr; 
   }*/
   // using loop ref voltage as specified in RF6886 datasheet
   // analogWrite(pwr_control_pin, pwr);
}


bool HAMShield::frequency(uint32_t freq_khz) {  
  if((freq_khz >= 137000) && (freq_khz <= 174000)) { 
      setVHF();
      setBand(3); // 0b11 is 134-174MHz
      setFrequency(freq_khz);
      return true;
  }
  
  if((freq_khz >= 200000) && (freq_khz <= 260000)) { 
      setVHF();
      setBand(2); // 10 is 200-260MHz
      setFrequency(freq_khz);
      return true;
  }
  
  if((freq_khz >= 400000) && (freq_khz <= 520000)) { 
      setUHF();
      setBand(00); // 00 is 400-520MHz
      setFrequency(freq_khz); 
      return true;
  }
  return false;
}

/* FRS Lookup Table */

bool HAMShield::setFRSChannel(uint8_t channel) { 
  if(channel < 15) { 
    setFrequency(FRS[channel]);
    return true;
  }
  return false;
} 

/* GMRS Lookup Table (borrows from FRS table since channels overlap) */

bool HAMShield::setGMRSChannel(uint8_t channel) { 
  if((channel > 8) & (channel < 16)) { 
     channel = channel - 7;           // we start with 0, to try to avoid channel 8 being nothing
     setFrequency(FRS[channel]);
     return true; 
  }
  if(channel < 9) { 
     setFrequency(GMRS[channel]);
     return true;
  }
  return false;
}

/* MURS band is 11.25KHz (2.5KHz dev) in channel 1-3, 20KHz (5KHz dev) in channel 4-5. Should we set this? */

bool HAMShield::setMURSChannel(uint8_t channel) { 
  if(channel < 6) { 
     setFrequency(MURS[channel]); 
     return true;
  }
}

/* Weather radio channels */

bool HAMShield::setWXChannel(uint8_t channel) { 
  if(channel < 8) { 
     setFrequency(WX[channel]);
     setModeReceive();
     // turn off squelch?
     // channel bandwidth? 
     return true;
  }
  return false;
}

/* Scan channels for strongest signal. returns channel number. You could do radio.setWXChannel(radio.scanWXChannel()) */

uint8_t HAMShield::scanWXChannel() { 
  uint8_t channel = 0;
  int16_t toprssi = 0;
  for(int x = 0; x < 8; x++) { 
     setWXChannel(x);
     delay(100);
     int16_t rssi = readRSSI();
     if(rssi > toprssi) { toprssi = rssi; channel = x; }
  }
  return channel;
}


/* removes the out of band transmit restrictions for those who hold special licenses */

void HAMShield::dangerMode() { 
  restrictions = false;
  return;
} 

/* enable restrictions on out of band transmissions */

void HAMShield::safeMode() { 
  restrictions = true;
  return;
}

/* scanner mode. Scans a range and returns the active frequency when it detects a signal. If none is detected, returns 0. */

uint32_t HAMShield::scanMode(uint32_t start,uint32_t stop, uint8_t speed, uint16_t step, uint16_t threshold) { 
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

uint32_t HAMShield::findWhitespace(uint32_t start,uint32_t stop, uint8_t dwell, uint16_t step, uint16_t threshold) { 
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

uint32_t HAMShield::scanChannels(uint32_t buffer[],uint8_t buffsize, uint8_t speed, uint16_t threshold) { 
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

uint32_t HAMShield::findWhitespaceChannels(uint32_t buffer[],uint8_t buffsize, uint8_t dwell, uint16_t threshold) { 
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
BUG: I cannot figure out how to attach these interrupt handlers without the error: 

/Users/casey/Documents/Arduino/libraries/HAMShield/HAMShield.cpp: In member function 'void HAMShield::buttonMode(uint8_t)':
/Users/casey/Documents/Arduino/libraries/HAMShield/HAMShield.cpp:1125: error: argument of type 'void (HAMShield::)()' does not match 'void (*)()'
/Users/casey/Documents/Arduino/libraries/HAMShield/HAMShield.cpp:1126: error: argument of type 'void (HAMShield::)()' does not match 'void (*)()'
*/


/*
void HAMShield::buttonMode(uint8_t mode) { 
   pinMode(HAMSHIELD_AUX_BUTTON,INPUT);       // set the pin mode to input
   digitalWrite(HAMSHIELD_AUX_BUTTON,HIGH);   // turn on internal pull up 
   if(mode == PTT_MODE) { attachInterrupt(HAMSHIELD_AUX_BUTTON, isr_ptt, CHANGE); } 
   if(mode == RESET_MODE) { attachInterrupt(HAMSHIELD_AUX_BUTTON, isr_reset, CHANGE); }
}
*/

/* Interrupt routines */

/* handle aux button to reset condition */

void HAMShield::isr_reset() { 
  wdt_enable(WDTO_15MS);
  while(1) { } 
}

/* Transmit on press, receive on release. We need debouncing !! */

void HAMShield::isr_ptt() { 
   if((bouncer + 200) > millis()) { 
   if(ptt == false) { 
      ptt = true;
      HAMShield::setModeTransmit();
      bouncer = millis();
   }
   if(ptt == true) { 
      ptt = false;
      HAMShield::setModeReceive();
      bouncer = millis();
   } }
}

/* 

 Radio etiquette function: Wait for empty channel.

 Optional timeout (0 waits forever)
 Optional break window (how much dead air to wait for after a transmission completes)

Does not take in account the millis() overflow 
   
*/

bool HAMShield::waitForChannel(long timeout = 0, long breakwindow = 0) { 
    int16_t rssi = 0;                                                              // Set RSSI to max received signal
    for(int x = 0; x < 20; x++) { rssi = readRSSI(); }                            // "warm up" to get past RSSI hysteresis 
    long timer = millis() + timeout;                                              // Setup the timeout value
    if(timeout == 0) { timer = 4294967295; }                                      // If we want to wait forever, set it to the max millis()
    while(timer > millis()) {                                                     // while our timer is not timed out.
        rssi = readRSSI();                                                        // Read signal strength
        if(rssi < HAMSHIELD_EMPTY_CHANNEL_RSSI) {                                 // If the channel is empty, lets see if anyone breaks in.
             timer = millis() + breakwindow;
             while(timer > millis()) {
                 rssi = readRSSI();
                 if(rssi > HAMSHIELD_EMPTY_CHANNEL_RSSI) { return false; }        // Someone broke into the channel, abort.
             } return true;                                                       // It passed the test...channel is open.
        }
    }
    return false;
}


/* Morse code out, blocking */

void HAMShield::morseOut(char buffer[HAMSHIELD_MORSE_BUFFER_SIZE]) { 

 for(int x = 0; x < strlen(buffer); x++) {
  char output = morseLookup(buffer[x]); 
  if(buffer[x] != ' ') { 
  for(int m = 0; m < strlen(itu[output]); m++) {
     if(itu[output][m] == '-') { tone(HAMSHIELD_PWM_PIN,1000,HAMSHIELD_MORSE_DOT*3); delay(HAMSHIELD_MORSE_DOT*3); }
     else { tone(HAMSHIELD_PWM_PIN,1000,HAMSHIELD_MORSE_DOT); delay(HAMSHIELD_MORSE_DOT); }
     delay(HAMSHIELD_MORSE_DOT);
     }
     delay(HAMSHIELD_MORSE_DOT*3);
  } else { delay(HAMSHIELD_MORSE_DOT*7); } 
 }
 return;
}

/* Morse code lookup table */

char HAMShield::morseLookup(char letter) { 
 for(int x = 0; x < 54; x++) { 
  if(letter == ascii[x]) { 
    return x; 
   // return itu[x];
  } 
 }
}

/* 

SSTV VIS Digital Header 

Reference: http://www.barberdsp.com/files/Dayton%20Paper.pdf

Millis	Freq	Description
-----------------------------------------------
300 	1900 	Leader tone
10 	1200 	break
300 	1900 	Leader tone
30 	1200 	VIS start bit
30 	bit 0	1100hz = “1”, 1300hz = “0”
30 	bit 1	“”
30 	bit 2 	“”
30	bit 3 	“”
30 	bit 4 	“”
30 	bit 5 	“”
30 	bit 6 	“”
30	PARITY	Even=1300hz,Odd=1100hz
30	1200	VIS stop bit

*/

void HAMShield::SSTVVISCode(int code) { 
	toneWait(1900,300);
	toneWait(1200,10);
	toneWait(1900,300);
	toneWait(1200,30);
        for(int x = 0; x < 7; x++) { 
           if(bitRead(code,x)) { toneWait(1100,30); } else { toneWait(1300,30); } 
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

void HAMShield::SSTVTestPattern(int code) { 
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

void HAMShield::toneWait(uint16_t freq, long timer) { 
    tone(HAMSHIELD_PWM_PIN,freq,timer);
    delay(timer);
}

/* wait microseconds for tone to complete */

void HAMShield::toneWaitU(uint16_t freq, long timer) { 
    if(freq < 16383) { 
    tone(HAMSHIELD_PWM_PIN,freq);
    delayMicroseconds(timer); noTone(HAMSHIELD_PWM_PIN); return;
    }
    tone(HAMSHIELD_PWM_PIN,freq);
    delay(timer / 1000); noTone(HAMSHIELD_PWM_PIN); return;
}


bool HAMShield::parityCalc(int code) {  
     unsigned int v;       // word value to compute the parity of
     bool parity = false;  // parity will be the parity of v

    while (code)
    {
       parity = !parity;
       code = code & (code - 1);
    }

    return parity;
}
/*
void HAMShield::AFSKOut(char buffer[80]) { 
  for(int x = 0; x < 65536; x++) { 
  startPlayback(AFSK_mark, sizeof(AFSK_mark));  delay(8); 
  startPlayback(AFSK_space, sizeof(AFSK_space));  delay(8); }

}
*/