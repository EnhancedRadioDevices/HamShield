// HamShield library collection
// Based on Programming Manual rev. 2.0, 5/19/2011 (RM-MPU-6000A-00)
// 11/22/2013 by Morgan Redfield <redfieldm@gmail.com>
// 04/26/2015 various changes Casey Halverson <spaceneedle@gmail.com>
// 05/08/2017 CTCSS code added


#ifndef _HAMSHIELD_H_
#define _HAMSHIELD_H_

#include "HamShield_comms.h"

// HamShield constants

#define HAMSHIELD_MORSE_BUFFER_SIZE      80    // Char buffer size for morse code text
#define HAMSHIELD_EMPTY_CHANNEL_RSSI   -110    // Default threshold where channel is considered "clear"

// Device Registers
#define A1846S_CTL_REG              0x30    // control register
#define A1846S_CLK_MODE_REG         0x04    // clk_mode
#define A1846S_PABIAS_REG           0x0A    // control register for bias voltage
//#define A1846S_BAND_SEL_REG         0x0F    // band_sel register <1:0>
#define A1846S_FLAG_REG             0x1C
#define A1846S_GPIO_MODE_REG        0x1F    // GPIO mode select register
#define A1846S_FREQ_HI_REG          0x29    // freq<29:16>
#define A1846S_FREQ_LO_REG          0x2A    // freq<15:0>
//#define A1846S_XTAL_FREQ_REG        0x2B    // xtal_freq<15:0>
//#define A1846S_ADCLK_FREQ_REG       0x2C    // adclk_freq<15:0>
#define A1846S_INT_MODE_REG         0x2D    // interrupt enables
#define A1846S_TX_VOICE_REG         0x3A    // tx voice control reg
#define A1846S_TH_H_VOX_REG         0x64    // register holds vox high (open) threshold bits
#define A1846S_TH_L_VOX_REG         0x64    // register holds vox low (shut) threshold bits
#define A1846S_FM_DEV_REG           0x43    // register holds fm deviation settings
#define A1846S_RX_VOLUME_REG        0x44    // register holds RX volume settings
#define A1846S_SQ_OPEN_THRESH_REG   0x49    // see sq
#define A1846S_SQ_SHUT_THRESH_REG   0x49    // see sq
#define A1846S_CTCSS_FREQ_REG       0x4A    // ctcss_freq<15:0>
#define A1846S_CDCSS_CODE_HI_REG    0x4B    // cdcss_code<23:16>
#define A1846S_CDCSS_CODE_LO_REG    0x4C    // cdccs_code<15:0>
#define A1846S_CTCSS_MODE_REG       0x4e    // see ctcss
#define A1846S_SQ_OUT_SEL_REG       0x54    // see sq
#define A1846S_FILTER_REG      0x58
#define A1846S_CTCSS_THRESH_REG     0x5B
#define A1846S_RSSI_REG             0x1B    // holds RSSI (unit 1dB)
#define A1846S_VSSI_REG             0x1A    // holds VSSI (unit mV)

#define A1846S_DTMF_ENABLE_REG      0x7A    // holds dtmf_enable
#define A1846S_DTMF_CODE_REG        0x7E    // holds dtmf_sample and dtmf_code
#define A1846S_TONE1_FREQ           0x35    // holds frequency of tone 1 (in 0.1Hz increments)
#define A1846S_TONE2_FREQ           0x36    // holds frequency of tone 2 (in 0.1Hz increments)
#define A1846S_DTMF_TIME_REG        0x7B    // holds time intervals for DTMF

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

// Bitfields for A1846_GPIO_MODE_REG
#define A1846S_GPIO7_MODE_BIT     15  // <1:0> 00=hi-z,01=vox,10=low,11=hi
#define A1846S_GPIO7_MODE_LENGTH   2
#define A1846S_GPIO6_MODE_BIT     13  // <1:0> 00=hi-z,01=sq or =sq&ctcss/cdcss when sq_out_sel=1,10=low,11=hi
#define A1846S_GPIO6_MODE_LENGTH   2
#define A1846S_GPIO5_MODE_BIT     11  // <1:0> 00=hi-z,01=txon_rf,10=low,11=hi
#define A1846S_GPIO5_MODE_LENGTH   2
#define A1846S_GPIO4_MODE_BIT      9  // <1:0> 00=hi-z,01=rxon_rf,10=low,11=hi
#define A1846S_GPIO4_MODE_LENGTH   2
#define A1846S_GPIO3_MODE_BIT      7  // <1:0> 00=hi-z,01=sdo,10=low,11=hi
#define A1846S_GPIO3_MODE_LENGTH   2
#define A1846S_GPIO2_MODE_BIT      5  // <1:0> 00=hi-z,01=int,10=low,11=hi
#define A1846S_GPIO2_MODE_LENGTH   2
#define A1846S_GPIO1_MODE_BIT      3  // <1:0> 00=hi-z,01=code_out/code_in,10=low,11=hi
#define A1846S_GPIO1_MODE_LENGTH   2
#define A1846S_GPIO0_MODE_BIT      1  // <1:0> 00=hi-z,01=css_out/css_in/css_cmp,10=low,11=hi
#define A1846S_GPIO0_MODE_LENGTH   2

// Bitfields for A1846S_INT_MODE_REG
#define A1846S_CSS_CMP_INT_BIT          9  // css_cmp_uint16_t enable
#define A1846S_RXON_RF_INT_BIT          8  // rxon_rf_uint16_t enable
#define A1846S_TXON_RF_INT_BIT          7  // txon_rf_uint16_t enable
#define A1846S_CTCSS_PHASE_INT_BIT      5  // ctcss phase shift detect uint16_t enable
#define A1846S_IDLE_TIMEOUT_INT_BIT     4  // idle state time out uint16_t enable
#define A1846S_RXON_RF_TIMEOUT_INT_BIT  3  // rxon_rf timerout uint16_t enable
#define A1846S_SQ_INT_BIT               2  // sq uint16_t enable
#define A1846S_TXON_RF_TIMEOUT_INT_BIT  1  // txon_rf time out uint16_t enable
#define A1846S_VOX_INT_BIT              0  // vox uint16_t enable

// Bitfields for A1846S_TX_VOICE_REG
#define A1846S_VOICE_SEL_BIT      14  //voice_sel<1:0>
#define A1846S_VOICE_SEL_LENGTH    3
#define A1846S_CTCSS_DET_BIT       5

// Bitfields for A1846S_TH_H_VOX_REG
#define A1846S_TH_H_VOX_BIT       13  // th_h_vox<13:7>
#define A1846S_TH_H_VOX_LEN       7

// Bitfields for A1846S_TH_L_VOX_REG
#define A1846S_TH_L_VOX_BIT       6  // th_l_vox<6:0>
#define A1846S_TH_L_VOX_LEN       7

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

// Bitfields for Sub Audio Bits
#define A1846S_CTDCSS_OUT_SEL_BIT  5
#define A1846S_CTDCSS_DTEN_BIT     4
#define A1846S_CTDCSS_DTEN_LEN     5
#define A1846S_CDCSS_SEL_BIT       6  // cdcss_sel
#define A1846S_CDCSS_INVERT_BIT    6  // cdcss_sel
#define A1846S_SHIFT_SEL_BIT      15
#define A1846S_SHIFT_SEL_LEN       2

// Bitfields for A1846S_SQ_THRESH_REG
#define A1846S_SQ_OPEN_THRESH_BIT    13  // sq open threshold <6:0>
#define A1846S_SQ_OPEN_THRESH_LENGTH  7

// Bitfields for A1846S_SQ_SHUT_THRESH_REG
#define A1846S_SQ_SHUT_THRESH_BIT     6  // sq shut threshold <6:0>
#define A1846S_SQ_SHUT_THRESH_LENGTH  7

// Bitfields for A1846S_SQ_OUT_SEL_REG
#define A1846S_SQ_OUT_SEL_BIT      7  // sq_out_sel

// Bitfields for A1846S_FILTER_REG
#define A1846S_VXHPF_FILTER_EN     11
#define A1846S_VXLPF_FILTER_EN     12
#define A1846S_EMPH_FILTER_EN      7
#define A1846S_VHPF_FILTER_EN      6
#define A1846S_VLPF_FILTER_EN      5
#define A1846S_CTCSS_FILTER_BYPASS   3

// Bitfields for A1846S_FLAG_REG
#define A1846S_CTCSS1_FLAG_BIT     9  // 1 when rxon is enabled
#define A1846S_CTCSS2_FLAG_BIT     8  // 1 when txon is enabled
#define A1846S_INVERT_DET_FLAG_BIT 7  // ctcss phase shift detect
#define A1846S_CSS_CMP_FLAG_BIT    2  // ctcss/cdcss compared
#define A1846S_SQ_FLAG_BIT         0  // sq final signal out from dsp
#define A1846S_VOX_FLAG_BIT        1  // vox out from dsp

// Bitfields for A1846S_RSSI_REG
#define A1846S_RSSI_BIT            15  // RSSI readings <7:0>
#define A1846S_RSSI_LENGTH         8

// Bitfields for A1846S_VSSI_REG
#define A1846S_VSSI_BIT           15  // voice signal strength indicator <7:0> (unit 0.5dB)
#define A1846S_VSSI_LENGTH        8
#define A1846S_MSSI_BIT           7  // mic signal strength <7:0> (unit 0.5 dB)
#define A1846S_MSSI_LENGTH        8

// Bitfields for A1846S_DTMF_ENABLE_REG
#define A1846S_DTMF_ENABLE_BIT    15
#define A1846S_TONE_DETECT        14
#define A18462_DTMF_DET_TIME_BIT   7
#define A18462_DTMF_DET_TIME_LEN   8

// Bitfields for A1846S_DTMF_SAMPLE_REG
#define A1846S_DTMF_SAMPLE_BIT    4
#define A1846S_DTMF_CODE_BIT      3
#define A1846S_DTMF_CODE_LEN      4
#define A1846S_DTMF_TX_IDLE_BIT   5

// Bitfields for A1846S_DTMF_TIME_REG
#define A1846S_DUALTONE_TX_TIME_BIT 5 // duration of dual tone TX (via DTMF) in 2.5ms increments
#define A1846S_DUALTONE_TX_TIME_LEN 6 
#define A1846S_DTMF_IDLE_TIME_BIT 11
#define A1846S_DTMF_IDLE_TIME_LEN 6

// SSTV VIS Codes


#define ROBOT8BW 2
#define SC2_180 55
#define MARTIN1 44

// RTTY Frequencies

#define HAMSHIELD_RTTY_FREQ 2200
#define HAMSHIELD_RTTY_SHIFT 850
#define HAMSHIELD_RTTY_BAUD 75

// PSK31 Frequencies

#define HAMSHIELD_PSK31_FREQ 1000


// Morse Configuration

#define MORSE_FREQ 600
#define MORSE_DOT 150 // ms

#define SYMBOL_END_TIME 5 //millis
#define CHAR_END_TIME (MORSE_DOT*2.7)
#define MESSAGE_END_TIME (MORSE_DOT*8)

#define MIN_DOT_TIME (MORSE_DOT-30)
#define MAX_DOT_TIME (MORSE_DOT+55)
#define MIN_DASH_TIME (MORSE_DOT*3-30)
#define MAX_DASH_TIME (MORSE_DOT*3+55)


class HamShield {
    public:
		HamShield(uint8_t ncs_pin = nCS, uint8_t clk_pin = CLK, uint8_t dat_pin = DAT, uint8_t mic_pin = MIC);

        void initialize();  // defaults to 12.5kHz
		void initialize(bool narrowBand); // select 12.5kHz if true or 25kHz if false
		void setupWideBand();
		void setupNarrowBand();
        bool testConnection();
        
        // read control reg
        uint16_t readCtlReg();
        void softReset();
		
        // restrictions control
        void dangerMode();
        void safeMode();
		
		bool frequency(uint32_t freq_khz);
        bool frequency_float(float freq_khz);
		uint32_t getFrequency();
		float getFrequency_float();
		
                /* ToDo	
		// channel mode
		// 11 - 25kHz channel
		// 00 - 12.5kHz channel
		// 10,01 - reserved
		void setChanMode(uint16_t mode);
		uint16_t getChanMode();
		*/
	
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
		
		/*
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
		*/
		
		// Subaudio settings
		
		//   Ctcss/cdcss mode sel
		void setCtcssCdcssMode(uint16_t mode);
		uint16_t getCtcssCdcssMode();
		void setDetPhaseShift();
		void setDetInvertCdcss();
		void setDetCdcss();
		void setDetCtcss();
		void disableCtcssCdcss();
		
		// ctcss settings
		void setCtcss(float freq_Hz);
		void setCtcssFreq(uint16_t freq_milliHz);
		uint16_t getCtcssFreqMilliHz();
		float getCtcssFreqHz();
		void setCtcssFreqToStandard(); // freq must be 134.4Hz for standard cdcss mode
        void enableCtcssTx();
        void enableCtcssRx();
		void enableCtcss();
        void disableCtcssTx();
        void disableCtcssRx();
		void disableCtcss();
		void setCtcssDetThreshIn(uint8_t thresh);
		uint8_t getCtcssDetThreshIn();
		void setCtcssDetThreshOut(uint8_t thresh);
		uint8_t getCtcssDetThreshOut();
		bool getCtcssToneDetected();

		//   Ctcss_sel
		//      1 = ctcss_cmp/cdcss_cmp out via gpio
		//      0 = ctcss/cdcss sdo out vio gpio
		void setCtcssGpioSel(bool cmp_nsdo);
		bool getCtcssGpioSel();
		
		void setCdcssInvert(bool invert);
		bool getCdcssInvert();
		
		//   Cdcss_sel
		//      1 = long (24 bit) code
		//      0 = short(23 bit) code
		void setCdcssSel(bool long_nshort);
		bool getCdcssSel();
		// Cdcss neg_det_en
		bool getCdcssNegDetEnabled();
		
		// Cdcss pos_det_en
		bool getCdcssPosDetEnabled();

		// ctss_det_en
		bool getCtssDetEnabled();
		
		// cdcss codes
		void setCdcssCode(uint16_t code);
		uint16_t getCdcssCode();
		
		// SQ
		void setSQOn();
		void setSQOff();
		bool getSQState();
		
		// SQ threshold
		void setSQHiThresh(int16_t sq_hi_threshold); // Sq detect high th, rssi_cmp will be 1 when rssi>th_h_sq, unit 1dB
		int16_t getSQHiThresh();
		void setSQLoThresh(int16_t sq_lo_threshold); // Sq detect low th, rssi_cmp will be 0 when rssi<th_l_sq && time delay meet, unit 1dB
		int16_t getSQLoThresh();
        bool getSquelching();
		
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
		//   Reading a single DTMF code:
		//     enableDTMFReceive()
		//     while (getDTMFSample() == 0) { delay(10); }
		//     uint16_t code = getDTMFCode();
		//     while (getDTMFSample() == 1) { delay(10); }
		//     disableDTMF();
        //   Writing a single DTMF code:
        //     setDTMFCode(code); // code is a uint16_t from 0x0 to 0xF
		void enableDTMFReceive();
		void setDTMFDetectTime(uint16_t detect_time);
        uint16_t getDTMFDetectTime();
        void setDTMFIdleTime(uint16_t idle_time); // idle time is time between DTMF Tone
        uint16_t getDTMFIdleTime();
        char DTMFRxLoop();
        char DTMFcode2char(uint16_t code);
        uint8_t DTMFchar2code(char c);
        void setDTMFTxTime(uint16_t tx_time); // tx time is duration of DTMF Tone
        uint16_t getDTMFTxTime();
		uint16_t disableDTMF();
		uint16_t getDTMFSample();
		uint16_t getDTMFCode();
        uint16_t getDTMFTxActive();
        void setDTMFCode(uint16_t code);
		
		// Tone
        void HStone(uint8_t pin, unsigned int frequency);
        void HSnoTone(uint8_t pin);
		void lookForTone(uint16_t tone_hz);
		uint8_t toneDetected();
        

		// TX FM deviation
		void setFMVoiceCssDeviation(uint16_t deviation);
		uint16_t getFMVoiceCssDeviation();
		void setFMCssDeviation(uint16_t deviation);
		uint16_t getFMCssDeviation();
		
		// RX voice range
		void setMute();
		void setUnmute();
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
		void setGpios(uint16_t mode);
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

		// Voice filters
		void bypassVoiceHpf();
		void useVoiceHpf();
		bool getVoiceHpfEnabled();
		void bypassVoiceLpf();
		void useVoiceLpf();
		bool getVoiceLpfEnabled();

		// Vox filters
		void bypassVoxHpf();
		void useVoxHpf();
		bool getVoxHpfEnabled();
		void bypassVoxLpf();
		void useVoxLpf();
		bool getVoxLpfEnabled();
		
		// Read Only Status Registers
		int16_t readRSSI();
		uint16_t readVSSI();
        uint16_t readMSSI();

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

        void setupMorseRx();
		unsigned int getMorseFreq();
		void setMorseFreq(unsigned int morse_freq_hz);
		unsigned int getMorseDotMillis();
		void setMorseDotMillis(unsigned int morse_dot_dur_millis);
		void morseOut(char buffer[HAMSHIELD_MORSE_BUFFER_SIZE]);
        char morseRxLoop();
        bool handleMorseTone(uint16_t tone_time, bool bits_to_process, uint8_t * rx_morse_char, uint8_t * rx_morse_bit);
        char parseMorse(uint8_t rx_morse_char, uint8_t rx_morse_bit);
		uint8_t morseLookup(char letter);
		uint8_t morseReverseLookup(uint8_t itu);
        bool waitForChannel(long timeout, long breakwindow, int setRSSI);
        void SSTVVISCode(int code);
        void SSTVTestPattern(int code);
        void toneWait(uint16_t freq, long timer);
        void toneWaitU(uint16_t freq, long timer);
        bool parityCalc(int code);
		
		
		
    private:
        uint8_t devAddr;
        uint8_t hs_mic_pin;
        uint16_t radio_i2c_buf[4];
		bool tx_active;
		bool rx_active;
        float radio_frequency;
/*        uint32_t FRS[];
        uint32_t GMRS[];
        uint32_t MURS[];
        uint32_t WX[];
*/		
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
