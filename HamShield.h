// HamShield library collection
// Based on Programming Manual rev. 2.0, 5/19/2011 (RM-MPU-6000A-00)
// 11/22/2013 by Morgan Redfield <redfieldm@gmail.com>
// 04/26/2015 various changes Casey Halverson <spaceneedle@gmail.com>



#ifndef _HAMSHIELD_H_
#define _HAMSHIELD_H_

//#include "I2Cdev_rda.h"
//#include "I2Cdev.h"
#include "HamShield_comms.h"
#include "SimpleFIFO.h"
#include "AFSK.h"
#include "DDS.h"
#include <avr/pgmspace.h>

// HamShield constants

#define HAMSHIELD_MORSE_DOT             100    // Morse code dot length (smaller is faster WPM)
#define HAMSHIELD_MORSE_BUFFER_SIZE      80    // Char buffer size for morse code text
#define HAMSHIELD_AUX_BUTTON              2    // Pin assignment for AUX button
#define HAMSHIELD_PWM_PIN                 3    // Pin assignment for PWM output
#define HAMSHIELD_EMPTY_CHANNEL_RSSI   -110    // Default threshold where channel is considered "clear"

#define HAMSHIELD_AFSK_RX_FIFO_LEN 16

// button modes
#define PTT_MODE 1
#define RESET_MODE 2

// Device Constants
#define A1846S_DEV_ADDR_SENHIGH 0b0101110
#define A1846S_DEV_ADDR_SENLOW  0b1110001


// Device Registers
#define A1846S_CTL_REG              0x30    // control register
#define A1846S_CLK_MODE_REG         0x04    // clk_mode
#define A1846S_PABIAS_REG           0x0A    // control register for bias voltage
//#define A1846S_BAND_SEL_REG         0x0F    // band_sel register <1:0>
#define A1846S_GPIO_MODE_REG        0x1F    // GPIO mode select register
#define A1846S_FREQ_HI_REG          0x29    // freq<29:16>
#define A1846S_FREQ_LO_REG          0x2A    // freq<15:0>
//#define A1846S_XTAL_FREQ_REG        0x2B    // xtal_freq<15:0>
//#define A1846S_ADCLK_FREQ_REG       0x2C    // adclk_freq<15:0>
#define A1846S_INT_MODE_REG         0x2D    // interrupt enables
#define A1846S_TX_VOICE_REG         0x3A    // tx voice control reg
#define A1846S_TH_H_VOX_REG         0x41    // register holds vox high (open) threshold bits
#define A1846S_TH_L_VOX_REG         0x42    // register holds vox low (shut) threshold bits
#define A1846S_FM_DEV_REG           0x43    // register holds fm deviation settings
#define A1846S_RX_VOLUME_REG        0x44    // register holds RX volume settings
#define A1846S_SUBAUDIO_REG         0x45    // sub audio register
#define A1846S_SQ_OPEN_THRESH_REG   0x48    // see sq
#define A1846S_SQ_SHUT_THRESH_REG   0x49    // see sq
#define A1846S_CTCSS_FREQ_REG       0x4A    // ctcss_freq<15:0>
#define A1846S_CDCSS_CODE_HI_REG    0x4B    // cdcss_code<23:16>
#define A1846S_CDCSS_CODE_LO_REG    0x4C    // cdccs_code<15:0>
#define A1846S_SQ_OUT_SEL_REG       0x54    // see sq
#define A1846S_EMPH_FILTER_REG      0x58
#define A1846S_FLAG_REG             0x5C    // holds flags for different statuses
#define A1846S_RSSI_REG             0x1B    // holds RSSI (unit 1dB)
#define A1846S_VSSI_REG             0x1A    // holds VSSI (unit mV)
#define A1846S_DTMF_CTL_REG         0x63    // see dtmf
#define A1846S_DTMF_C01_REG         0x66    // holds frequency value for c0 and c1
#define A1846S_DTMF_C23_REG         0x67    // holds frequency value for c2 and c3
#define A1846S_DTMF_C45_REG         0x68    // holds frequency value for c4 and c5
#define A1846S_DTMF_C67_REG         0x69    // holds frequency value for c6 and c7
#define A1846S_DTMF_RX_REG          0x6C    // received dtmf signal

// NOTE: could add registers and bitfields for dtmf tones, is this necessary?


// Device Bit Fields

// Bitfields for A1846S_CTL_REG
#define A1846S_CHAN_MODE_BIT      13  //channel_mode<1:0> 
#define A1846S_CHAN_MODE_LENGTH    2
#define A1846S_TAIL_ELIM_EN_BIT   11  // enables tail elim when set to 1
#define A1846S_ST_MODE_BIT         9  // set mode for txon and rxon
#define A1846S_ST_MODE_LENGTH      2
#define A1846S_MUTE_BIT            7  // 0 no mute, 1 mute when rxno
#define A1846S_TX_MODE_BIT         6  //tx-on
#define A1846S_RX_MODE_BIT         5  //rx-on
#define A1846S_VOX_ON_BIT          4  // 0 off, 1 on and chip auto-vox
#define A1846S_SQ_ON_BIT           3  // auto sq enable bit
#define A1846S_PWR_DWN_BIT         2  // power control bit
#define A1846S_CHIP_CAL_EN_BIT     1  // 0 cal disable, 1 cal enable
#define A1846S_SOFT_RESET_BIT      0  // 0 normal value, 1 reset all registers to normal value

// Bitfields for A1846S_CLK_MODE_REG
#define A1846S_CLK_MODE_BIT        0  // 0 24-28MHz, 1 12-14MHz

// Bitfields for A1846S_PABIAS_REG
#define A1846S_PABIAS_BIT          5  // pabias_voltage<5:0>
#define A1846S_PABIAS_LENGTH       6

#define A1846S_PADRV_BIT          14  // pabias_voltage<14:11>
#define A1846S_PADRV_LENGTH       4

// Bitfields for A1846S_BAND_SEL_REG
//#define A1846S_BAND_SEL_BIT        7  // band_sel<1:0>
//#define A1846S_BAND_SEL_LENGTH     2

// Bitfields for RDA1864_GPIO_MODE_REG
#define RDA1864_GPIO7_MODE_BIT     15  // <1:0> 00=hi-z,01=vox,10=low,11=hi
#define RDA1864_GPIO7_MODE_LENGTH   2
#define RDA1864_GPIO6_MODE_BIT     13  // <1:0> 00=hi-z,01=sq or =sq&ctcss/cdcss when sq_out_sel=1,10=low,11=hi
#define RDA1864_GPIO6_MODE_LENGTH   2
#define RDA1864_GPIO5_MODE_BIT     11  // <1:0> 00=hi-z,01=txon_rf,10=low,11=hi
#define RDA1864_GPIO5_MODE_LENGTH   2
#define RDA1864_GPIO4_MODE_BIT      9  // <1:0> 00=hi-z,01=rxon_rf,10=low,11=hi
#define RDA1864_GPIO4_MODE_LENGTH   2
#define RDA1864_GPIO3_MODE_BIT      7  // <1:0> 00=hi-z,01=sdo,10=low,11=hi
#define RDA1864_GPIO3_MODE_LENGTH   2
#define RDA1864_GPIO2_MODE_BIT      5  // <1:0> 00=hi-z,01=int,10=low,11=hi
#define RDA1864_GPIO2_MODE_LENGTH   2
#define RDA1864_GPIO1_MODE_BIT      3  // <1:0> 00=hi-z,01=code_out/code_in,10=low,11=hi
#define RDA1864_GPIO1_MODE_LENGTH   2
#define RDA1864_GPIO0_MODE_BIT      1  // <1:0> 00=hi-z,01=css_out/css_in/css_cmp,10=low,11=hi
#define RDA1864_GPIO0_MODE_LENGTH   2

// Bitfields for A1846S_INT_MODE_REG
#define A1846S_CSS_CMP_INT_BIT          9  // css_cmp_uint16_t enable
#define A1846S_RXON_RF_INT_BIT          8  // rxon_rf_uint16_t enable
#define A1846S_TXON_RF_INT_BIT          7  // txon_rf_uint16_t enable
#define A1846S_DTMF_IDLE_INT_BIT        6  // dtmf_idle_uint16_t enable
#define A1846S_CTCSS_PHASE_INT_BIT      5  // ctcss phase shift detect uint16_t enable
#define A1846S_IDLE_TIMEOUT_INT_BIT     4  // idle state time out uint16_t enable
#define A1846S_RXON_RF_TIMeOUT_INT_BIT  3  // rxon_rf timerout uint16_t enable
#define A1846S_SQ_INT_BIT               2  // sq uint16_t enable
#define A1846S_TXON_RF_TIMEOUT_INT_BIT  1  // txon_rf time out uint16_t enable
#define A1846S_VOX_INT_BIT              0  // vox uint16_t enable

// Bitfields for A1846S_TX_VOICE_REG
#define A1846S_VOICE_SEL_BIT      14  //voice_sel<1:0>
#define A1846S_VOICE_SEL_LENGTH    3

// Bitfields for A1846S_TH_H_VOX_REG
#define A1846S_TH_H_VOX_BIT       14  // th_h_vox<14:0>
#define A1846S_TH_H_VOX_LENGTH    15

// Bitfields for A1846S_FM_DEV_REG
#define A1846S_FM_DEV_VOICE_BIT   12  // CTCSS/CDCSS and voice deviation <6:0>
#define A1846S_FM_DEV_VOICE_LENGTH 7
#define A1846S_FM_DEV_CSS_BIT      5  // CTCSS/CDCSS deviation only <5:0>
#define A1846S_FM_DEV_CSS_LENGTH   6

// Bitfields for A1846S_RX_VOLUME_REG
#define A1846S_RX_VOL_1_BIT        7  // volume 1 <3:0>, (0000)-15dB~(1111)0dB, step 1dB
#define A1846S_RX_VOL_1_LENGTH     4
#define A1846S_RX_VOL_2_BIT        3  // volume 2 <3:0>, (0000)-15dB~(1111)0dB, step 1dB
#define A1846S_RX_VOL_2_LENGTH     4

// Bitfields for A1846S_SUBAUDIO_REG Sub Audio Register
#define A1846S_SHIFT_SEL_BIT      15  // shift_sel<1:0> see eliminating tail noise
#define A1846S_SHIFT_SEL_LENGTH    2
#define A1846S_POS_DET_EN_BIT     11  // if 1, cdcss code will be detected
#define A1846S_CSS_DET_EN_BIT     10  // 1 - sq detect will add ctcss/cdcss detect result and control voice output on or off
#define A1846S_NEG_DET_EN_BIT      7  // if 1, cdcss inverse code will be detected at same time
#define A1846S_CDCSS_SEL_BIT       4  // cdcss_sel
#define A1846S_CTCSS_SEL_BIT       3  // ctcss_sel
#define A1846S_C_MODE_BIT          2  // c_mode<2:0>
#define A1846S_C_MODE_LENGTH       3

// Bitfields for A1846S_SQ_THRESH_REG
#define A1846S_SQ_OPEN_THRESH_BIT     9  // sq open threshold <9:0>
#define A1846S_SQ_OPEN_THRESH_LENGTH 10

// Bitfields for A1846S_SQ_SHUT_THRESH_REG
#define A1846S_SQ_SHUT_THRESH_BIT     9  // sq shut threshold <9:0>
#define A1846S_SQ_SHUT_THRESH_LENGTH 10

// Bitfields for A1846S_SQ_OUT_SEL_REG
#define A1846S_SQ_OUT_SEL_BIT      7  // sq_out_sel

// Bitfields for A1846S_EMPH_FILTER_REG
#define A1846S_EMPH_FILTER_EN      3

// Bitfields for A1846S_FLAG_REG
#define A1846S_DTMF_IDLE_FLAG_BIT 12  // dtmf idle flag
#define A1846S_RXON_RF_FLAG_BIT   10  // 1 when rxon is enabled
#define A1846S_TXON_RF_FLAG_BIT    9  // 1 when txon is enabled
#define A1846S_INVERT_DET_FLAG_BIT 7  // ctcss phase shift detect
#define A1846S_CSS_CMP_FLAG_BIT    2  // ctcss/cdcss compared
#define A1846S_SQ_FLAG_BIT         1  // sq final signal out from dsp
#define A1846S_VOX_FLAG_BIT        0  // vox out from dsp

// Bitfields for A1846S_RSSI_REG
#define A1846S_RSSI_BIT            15  // RSSI readings <7:0>
#define A1846S_RSSI_LENGTH         8

// Bitfields for A1846S_VSSI_REG
#define A1846S_VSSI_BIT           14  // voice signal strength indicator <14:0> (unit mV)
#define A1846S_VSSI_LENGTH        15

// Bitfields for A1846S_DTMF_CTL_REG
#define A1846S_DTMF_MODE_BIT       9  // 
#define A1846S_DTMF_MODE_LENGTH    2
#define A1846S_DTMF_EN_BIT         8  // enable dtmf
#define A1846S_DTMF_TIME1_BIT      7  // dtmf time 1 <3:0>
#define A1846S_DTMF_TIME1_LENGTH   4
#define A1846S_DTMF_TIME2_BIT      3  // dtmf time 2 <3:0>
#define A1846S_DTMF_TIME2_LENGTH   4

// Bitfields for A1846S_DTMF_RX_REG
#define A1846S_DTMF_INDEX_BIT         10  // dtmf index <5:3> - tone 1 detect index, <2:0> - tone 2 detect index
#define A1846S_DTMF_INDEX_LENGTH       6
#define A1846S_DTMF_TONE1_IND_BIT     10
#define A1846S_DTMF_TONE1_IND_LENGTH   3
#define A1846S_DTMF_TONE2_IND_BIT      7
#define A1846S_DTMF_TONE2_IND_LENGTH   3
#define A1846S_DTMF_FLAG_BIT           4
#define A1846S_DTMF_CODE_BIT           3  // dtmf code out <3:0>
#define A1846S_DTMF_CODE_LENGTH        4
// dtmf code out
//  1:f0+f4, 2:f0+f5, 3:f0+f6, A:f0+f7,
//  4:f1+f4, 5:f1+f5, 6:f1+f6, B:f1+f7,
//  7:f2+f4, 8:f2+f5, 9:f2+f6, C:f2+f7,
//  E(*):f3+f4, 0:f3+f5, F(#):f3+f6, D:f3+f7

// Bitfields for DTMF registers
#define A1846S_DTMF_C0_BIT             15
#define A1846S_DTMF_C0_LENGTH           8
#define A1846S_DTMF_C1_BIT              7
#define A1846S_DTMF_C1_LENGTH           8
#define A1846S_DTMF_C2_BIT             15
#define A1846S_DTMF_C2_LENGTH           8
#define A1846S_DTMF_C3_BIT              7
#define A1846S_DTMF_C3_LENGTH           8
#define A1846S_DTMF_C4_BIT             15
#define A1846S_DTMF_C4_LENGTH           8
#define A1846S_DTMF_C5_BIT              7
#define A1846S_DTMF_C5_LENGTH           8
#define A1846S_DTMF_C6_BIT             15
#define A1846S_DTMF_C6_LENGTH           8
#define A1846S_DTMF_C7_BIT              7
#define A1846S_DTMF_C7_LENGTH           8

// SSTV VIS Codes


#define ROBOT8BW 2
#define SC2-180 55
#define MARTIN1 44

// RTTY Frequencies

#define HAMSHIELD_RTTY_FREQ 2200
#define HAMSHIELD_RTTY_SHIFT 850
#define HAMSHIELD_RTTY_BAUD 75

// PSK31 Frequencies

#define HAMSHIELD_PSK31_FREQ 1000



class HamShield {
    public:
	    // public singleton for ISRs to reference
        static HamShield *sHamShield; // HamShield singleton, used for ISRs mostly
		
        HamShield();
        HamShield(uint8_t address);

        void initialize();
        bool testConnection();
        
        // read control reg
        uint16_t readCtlReg();
        void softReset();
		
        // restrictions control
        void dangerMode();
        void safeMode();
		
		bool frequency(uint32_t freq_khz);
		uint32_t getFrequency();
			
		// channel mode
		// 11 - 25kHz channel
		// 00 - 12.5kHz channel
		// 10,01 - reserved
		void setChanMode(uint16_t mode);
		uint16_t getChanMode();
		
		void setModeTransmit(); // turn off rx, turn on tx
		void setModeReceive(); // turn on rx, turn off tx
		void setModeOff(); // turn off rx, turn off tx, set pwr_dwn bit
		
		// set tx source
		// 00 - Mic source
		// 01 - sine source from tone2
		// 10 - tx code from GPIO1 code_in (gpio1<1:0> must be set to 01)
		// 11 - no tx source
		void setTxSource(uint16_t tx_source);
		void setTxSourceMic();
		void setTxSourceTone1();
		void setTxSourceTone2();
		void setTxSourceTones();
		void setTxSourceNone();
		uint16_t getTxSource();
		
		// PA bias voltage is unused (maybe remove this)
		// set PA_bias voltage
		//    000000: 1.01V
		//    000001:1.05V
		//    000010:1.09V
		//    000100: 1.18V
		//    001000: 1.34V
		//    010000: 1.68V
		//    100000: 2.45V
		//    1111111:3.13V
		void setPABiasVoltage(uint16_t voltage);
		uint16_t getPABiasVoltage();
		
		// Subaudio settings
		
		//   Ctcss/cdcss mode sel
		//      x00=disable,
		//      001=inner ctcss en,
		//      010= inner cdcss en
		//      101= outer ctcss en,
		//      110=outer cdcss en
		//      others =disable
		void setCtcssCdcssMode(uint16_t mode);
		uint16_t getCtcssCdcssMode();
		void setInnerCtcssMode();
		void setInnerCdcssMode();
		void setOuterCtcssMode();
		void setOuterCdcssMode();
		void disableCtcssCdcss();
		
		//   Ctcss_sel
		//      1 = ctcss_cmp/cdcss_cmp out via gpio
		//      0 = ctcss/cdcss sdo out vio gpio
		void setCtcssSel(bool cmp_nsdo);
		bool getCtcssSel();
		
		//   Cdcss_sel
		//      1 = long (24 bit) code
		//      0 = short(23 bit) code
		void setCdcssSel(bool long_nshort);
		bool getCdcssSel();
		// Cdcss neg_det_en
		void enableCdcssNegDet();
		void disableCdcssNegDet();
		bool getCdcssNegDetEnabled();
		
		// Cdcss pos_det_en
		void enableCdcssPosDet();
		void disableCdcssPosDet();
		bool getCdcssPosDetEnabled();

		// css_det_en
		void enableCssDet();
		void disableCssDet();
		bool getCssDetEnabled();
		
		// ctcss freq
		void setCtcss(float freq);
		void setCtcssFreq(uint16_t freq);
		uint16_t getCtcssFreq();
		void setCtcssFreqToStandard(); // freq must be 134.4Hz for standard cdcss mode
		
		// cdcss codes
		void setCdcssCode(uint16_t code);
		uint16_t getCdcssCode();
				
		// SQ
		void setSQOn();
		void setSQOff();
		bool getSQState();
		
		// SQ threshold
		void setSQHiThresh(uint16_t sq_hi_threshold); // Sq detect high th, rssi_cmp will be 1 when rssi>th_h_sq, unit 1/8dB
		uint16_t getSQHiThresh();
		void setSQLoThresh(uint16_t sq_lo_threshold); // Sq detect low th, rssi_cmp will be 0 when rssi<th_l_sq && time delay meet, unit 1/8 dB
		uint16_t getSQLoThresh();
		
		// SQ out select
		void setSQOutSel();
		void clearSQOutSel();
		bool getSQOutSel();
		
		// VOX
		void setVoxOn();
		void setVoxOff();
		bool getVoxOn();
		
		// Vox Threshold
		void setVoxOpenThresh(uint16_t vox_open_thresh); // When vssi > th_h_vox, then vox will be 1(unit mV )
		uint16_t getVoxOpenThresh();
		void setVoxShutThresh(uint16_t vox_shut_thresh); // When vssi < th_l_vox && time delay meet, then vox will be 0 (unit mV )
		uint16_t getVoxShutThresh();
		
		// Tail Noise
		void enableTailNoiseElim();
		void disableTailNoiseElim();
		bool getTailNoiseElimEnabled();
		
		// tail noise shift select
		//   Select ctcss phase shift when use tail eliminating function when TX
		//     00 = 120 degree shift
		//     01 = 180 degree shift
		//     10 = 240 degree shift
		//     11 = reserved
		void setShiftSelect(uint16_t shift_sel);
		uint16_t getShiftSelect();
		
		// DTMF
		void setDTMFC0(uint16_t freq);
		uint16_t getDTMFC0();
		void setDTMFC1(uint16_t freq);
		uint16_t getDTMFC1();	
		void setDTMFC2(uint16_t freq);
		uint16_t getDTMFC2();
		void setDTMFC3(uint16_t freq);
		uint16_t getDTMFC3();
		void setDTMFC4(uint16_t freq);
		uint16_t getDTMFC4();
		void setDTMFC5(uint16_t freq);
		uint16_t getDTMFC5();
		void setDTMFC6(uint16_t freq);
		uint16_t getDTMFC6();
		void setDTMFC7(uint16_t freq);
		uint16_t getDTMFC7();
		
		// TX FM deviation
		void setFMVoiceCssDeviation(uint16_t deviation);
		uint16_t getFMVoiceCssDeviation();
		void setFMCssDeviation(uint16_t deviation);
		uint16_t getFMCssDeviation();
		
		// RX voice range
		void setVolume1(uint16_t volume);
		uint16_t getVolume1();
		void setVolume2(uint16_t volume);
		uint16_t getVolume2();
		
		// GPIO
		void setGpioMode(uint16_t gpio, uint16_t mode);
		void setGpioHiZ(uint16_t gpio);
		void setGpioFcn(uint16_t gpio);
		void setGpioLow(uint16_t gpio);
		void setGpioHi(uint16_t gpio);
		uint16_t getGpioMode(uint16_t gpio);
		uint16_t getGpios();
		
		// Int
		void enableInterrupt(uint16_t interrupt);
		void disableInterrupt(uint16_t interrupt);
		bool getInterruptEnabled(uint16_t interrupt);
		
		// ST mode
		void setStMode(uint16_t mode);
		uint16_t getStMode();
		void setStFullAuto();
		void setStRxAutoTxManu();
		void setStFullManu();
		
		// Pre-emphasis, De-emphasis filter
		void bypassPreDeEmph();
		void usePreDeEmph();
		bool getPreDeEmphEnabled();
		
		// Read Only Status Registers
		int16_t readRSSI();
		uint16_t readVSSI();
		uint16_t readDTMFIndex(); // may want to split this into two (index1 and index2)
		uint16_t readDTMFCode();

        // set output power of radio
        void setRfPower(uint8_t pwr);

        // channel helper functions
        bool setGMRSChannel(uint8_t channel);
        bool setFRSChannel(uint8_t channel);
        bool setMURSChannel(uint8_t channel);
        bool setWXChannel(uint8_t channel);
        uint8_t scanWXChannel();
  
        // utilities
        uint32_t scanMode(uint32_t start,uint32_t stop, uint8_t speed, uint16_t step, uint16_t threshold);
        uint32_t findWhitespace(uint32_t start,uint32_t stop, uint8_t dwell, uint16_t step, uint16_t threshold);
        uint32_t scanChannels(uint32_t buffer[],uint8_t buffsize, uint8_t speed, uint16_t threshold);
        uint32_t findWhitespaceChannels(uint32_t buffer[],uint8_t buffsize, uint8_t dwell, uint16_t threshold);
        void buttonMode(uint8_t mode);
        static void isr_ptt();
        static void isr_reset();
		void morseOut(char buffer[HAMSHIELD_MORSE_BUFFER_SIZE]);
		uint8_t morseLookup(char letter);
        bool waitForChannel(long timeout, long breakwindow, int setRSSI);
        void SSTVVISCode(int code);
        void SSTVTestPattern(int code);
        void toneWait(uint16_t freq, long timer);
        void toneWaitU(uint16_t freq, long timer);
        bool parityCalc(int code);
		// void AFSKOut(char buffer[80]); 

		// AFSK routines
		bool AFSKStart();
		bool AFSKEnabled() { return afsk.enabled(); }
		bool AFSKStop();
		bool AFSKOut(const char *);
       
		class AFSK afsk;
       
    private:
        uint8_t devAddr;
        uint16_t radio_i2c_buf[4];
		bool tx_active;
		bool rx_active;
        uint32_t radio_frequency;
        uint32_t FRS[];
        uint32_t GMRS[];
        uint32_t MURS[];
        uint32_t WX[];
		
		// private utility functions
		// these functions should not be called in the Arduino sketch
		// just use the above public functions to do everything
		
		void setFrequency(uint32_t freq_khz);
		void setTxBand2m();
		void setTxBand1_2m();
		void setTxBand70cm();
		
		// xtal frequency (kHz)
		// 12-14MHz crystal: this reg is set to crystal freq_khz
		// 24-28MHz crystal: this reg is set to crystal freq_khz / 2
		void setXtalFreq(uint16_t freq_kHz);
		uint16_t getXtalFreq();
			
		// adclk frequency (kHz)
		// 12-14MHz crystal: this reg is set to crystal freq_khz / 2
		// 24-28MHz crystal: this reg is set to crystal freq_khz / 4
		void setAdcClkFreq(uint16_t freq_kHz);
		uint16_t getAdcClkFreq();

		// clk mode
		// 12-14MHz: set to 1
		// 24-28MHz: set to 0
		void setClkMode(bool LFClk);
		bool getClkMode();
		
		// choose tx or rx
		void setTX(bool on_noff);
		bool getTX();
		
		void setRX(bool on_noff);
		bool getRX();
};

#endif /* _HAMSHIELD_H_ */
