#ifndef _DDS_H_
#define _DDS_H_

#include <avr/pgmspace.h>

// Use pin 3 for PWM? If not defined, use pin 11
// Quality on pin 3 is higher than on 11, as it can be clocked faster
// when the COMPARATOR_BITS value is less than 8
#define DDS_PWM_PIN_3

// Normally, we turn on timer2 and timer1, and have ADC sampling as our clock
// Define this to only use Timer2, and not start the ADC clock
// #define DDS_USE_ONLY_TIMER2

// Use a short (16 bit) accumulator. Phase accuracy is reduced, but speed
// is increased, along with a reduction in memory use.
#define SHORT_ACCUMULATOR

#ifdef SHORT_ACCUMULATOR
#define ACCUMULATOR_BITS      16
typedef uint16_t ddsAccumulator_t;
#else
#define ACCUMULATOR_BITS      32
typedef uint32_t ddsAccumulator_t;
#endif

// If defined, the timer will idle at 50% duty cycle
// This leaves it floating in the centre of the PWM/DAC voltage range
#define DDS_IDLE_HIGH

// Select how fast the PWM is, at the expense of level accuracy.
// A faster PWM rate will make for easier filtering of the output wave,
// while a slower one will allow for more accurate voltage level outputs,
// but will increase the filtering requirements on the output.
// 8 = 62.5kHz PWM
// 7 = 125kHz PWM
// 6 = 250kHz PWM
#ifdef DDS_PWM_PIN_3
#define COMPARATOR_BITS       6
#else // When using pin 11, we always want 8 bits
#define COMPARATOR_BITS       8
#endif

// This is how often we'll perform a phase advance, as well as ADC sampling
// rate. The higher this value, the smoother the output wave will be, at the
// expense of CPU time. It maxes out around 62000 (TBD)
// May be overridden in the sketch to improve performance
#ifndef DDS_REFCLK_DEFAULT
#define DDS_REFCLK_DEFAULT     38400
#endif
// As each Arduino crystal is a little different, this can be fine tuned to
// provide more accurate frequencies. Adjustments in the range of hundreds
// is a good start.
#ifndef DDS_REFCLK_OFFSET
#define DDS_REFCLK_OFFSET     0
#endif

#ifdef DDS_USE_ONLY_TIMER2
// TODO: Figure out where this clock value is generated from
#define DDS_REFCLK_DEFAULT     (62500/4)
#endif

// Output some of the calculations and information about the DDS over serial
#define DDS_DEBUG_SERIAL

// When defined, use the 1024 element sine lookup table. This improves phase
// accuracy, at the cost of more flash and CPU requirements.
// #define DDS_TABLE_LARGE

#ifdef DDS_TABLE_LARGE
// How many bits to keep from the accumulator to look up in this table
#define ACCUMULATOR_BIT_SHIFT (ACCUMULATOR_BITS-10)
static const uint8_t ddsSineTable[1024] PROGMEM = {
  128,128,129,130,131,131,132,133,134,135,135,136,137,138,138,139,
  140,141,142,142,143,144,145,145,146,147,148,149,149,150,151,152,
  152,153,154,155,155,156,157,158,158,159,160,161,162,162,163,164,
  165,165,166,167,167,168,169,170,170,171,172,173,173,174,175,176,
  176,177,178,178,179,180,181,181,182,183,183,184,185,186,186,187,
  188,188,189,190,190,191,192,192,193,194,194,195,196,196,197,198,
  198,199,200,200,201,202,202,203,203,204,205,205,206,207,207,208,
  208,209,210,210,211,211,212,213,213,214,214,215,215,216,217,217,
  218,218,219,219,220,220,221,221,222,222,223,224,224,225,225,226,
  226,227,227,228,228,228,229,229,230,230,231,231,232,232,233,233,
  234,234,234,235,235,236,236,236,237,237,238,238,238,239,239,240,
  240,240,241,241,241,242,242,242,243,243,243,244,244,244,245,245,
  245,246,246,246,246,247,247,247,248,248,248,248,249,249,249,249,
  250,250,250,250,250,251,251,251,251,251,252,252,252,252,252,252,
  253,253,253,253,253,253,253,254,254,254,254,254,254,254,254,254,
  254,254,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,254,
  254,254,254,254,254,254,254,254,254,254,253,253,253,253,253,253,
  253,252,252,252,252,252,252,251,251,251,251,251,250,250,250,250,
  250,249,249,249,249,248,248,248,248,247,247,247,246,246,246,246,
  245,245,245,244,244,244,243,243,243,242,242,242,241,241,241,240,
  240,240,239,239,238,238,238,237,237,236,236,236,235,235,234,234,
  234,233,233,232,232,231,231,230,230,229,229,228,228,228,227,227,
  226,226,225,225,224,224,223,222,222,221,221,220,220,219,219,218,
  218,217,217,216,215,215,214,214,213,213,212,211,211,210,210,209,
  208,208,207,207,206,205,205,204,203,203,202,202,201,200,200,199,
  198,198,197,196,196,195,194,194,193,192,192,191,190,190,189,188,
  188,187,186,186,185,184,183,183,182,181,181,180,179,178,178,177,
  176,176,175,174,173,173,172,171,170,170,169,168,167,167,166,165,
  165,164,163,162,162,161,160,159,158,158,157,156,155,155,154,153,
  152,152,151,150,149,149,148,147,146,145,145,144,143,142,142,141,
  140,139,138,138,137,136,135,135,134,133,132,131,131,130,129,128,
  128,127,126,125,124,124,123,122,121,120,120,119,118,117,117,116,
  115,114,113,113,112,111,110,110,109,108,107,106,106,105,104,103,
  103,102,101,100,100,99,98,97,97,96,95,94,93,93,92,91,
  90,90,89,88,88,87,86,85,85,84,83,82,82,81,80,79,
  79,78,77,77,76,75,74,74,73,72,72,71,70,69,69,68,
  67,67,66,65,65,64,63,63,62,61,61,60,59,59,58,57,
  57,56,55,55,54,53,53,52,52,51,50,50,49,48,48,47,
  47,46,45,45,44,44,43,42,42,41,41,40,40,39,38,38,
  37,37,36,36,35,35,34,34,33,33,32,31,31,30,30,29,
  29,28,28,27,27,27,26,26,25,25,24,24,23,23,22,22,
  21,21,21,20,20,19,19,19,18,18,17,17,17,16,16,15,
  15,15,14,14,14,13,13,13,12,12,12,11,11,11,10,10,
  10,9,9,9,9,8,8,8,7,7,7,7,6,6,6,6,
  5,5,5,5,5,4,4,4,4,4,3,3,3,3,3,3,
  2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,
  1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
  1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,
  2,3,3,3,3,3,3,4,4,4,4,4,5,5,5,5,
  5,6,6,6,6,7,7,7,7,8,8,8,9,9,9,9,
  10,10,10,11,11,11,12,12,12,13,13,13,14,14,14,15,
  15,15,16,16,17,17,17,18,18,19,19,19,20,20,21,21,
  21,22,22,23,23,24,24,25,25,26,26,27,27,27,28,28,
  29,29,30,30,31,31,32,33,33,34,34,35,35,36,36,37,
  37,38,38,39,40,40,41,41,42,42,43,44,44,45,45,46,
  47,47,48,48,49,50,50,51,52,52,53,53,54,55,55,56,
  57,57,58,59,59,60,61,61,62,63,63,64,65,65,66,67,
  67,68,69,69,70,71,72,72,73,74,74,75,76,77,77,78,
  79,79,80,81,82,82,83,84,85,85,86,87,88,88,89,90,
  90,91,92,93,93,94,95,96,97,97,98,99,100,100,101,102,
  103,103,104,105,106,106,107,108,109,110,110,111,112,113,113,114,
  115,116,117,117,118,119,120,120,121,122,123,124,124,125,126,127
};
#else
#define ACCUMULATOR_BIT_SHIFT (ACCUMULATOR_BITS-8)
static const int8_t ddsSineTable[256] PROGMEM = {
  0, 3, 6, 9, 12, 16, 19, 22, 25, 28, 31, 34, 37, 40, 43, 46, 49,
  51, 54, 57, 60, 63, 65, 68, 71, 73, 76, 78, 81, 83, 85, 88, 90,
  92, 94, 96, 98, 100, 102, 104, 106, 107, 109, 111, 112, 113, 115,
  116, 117, 118, 120, 121, 122, 122, 123, 124, 125, 125, 126, 126,
  126, 127, 127, 127, 127, 127, 127, 127, 126, 126, 126, 125, 125,
  124, 123, 122, 122, 121, 120, 118, 117, 116, 115, 113, 112, 111,
  109, 107, 106, 104, 102, 100, 98, 96, 94, 92, 90, 88, 85, 83, 81,
  78, 76, 73, 71, 68, 65, 63, 60, 57, 54, 51, 49, 46, 43, 40, 37,
  34, 31, 28, 25, 22, 19, 16, 12, 9, 6, 3, 0, -3, -6, -9, -12, -16,
  -19, -22, -25, -28, -31, -34, -37, -40, -43, -46, -49, -51, -54,
  -57, -60, -63, -65, -68, -71, -73, -76, -78, -81, -83, -85, -88,
  -90, -92, -94, -96, -98, -100, -102, -104, -106, -107, -109, -111,
  -112, -113, -115, -116, -117, -118, -120, -121, -122, -122, -123,
  -124, -125, -125, -126, -126, -126, -127, -127, -127, -127, -127,
  -127, -127, -126, -126, -126, -125, -125, -124, -123, -122, -122,
  -121, -120, -118, -117, -116, -115, -113, -112, -111, -109, -107,
  -106, -104, -102, -100, -98, -96, -94, -92, -90, -88, -85, -83,
  -81, -78, -76, -73, -71, -68, -65, -63, -60, -57, -54, -51, -49,
  -46, -43, -40, -37, -34, -31, -28, -25, -22, -19, -16, -12, -9, -6, -3
  };
/*static const uint8_t ddsSineTable[256] PROGMEM = {
  128,131,134,137,140,143,146,149,152,155,158,162,165,167,170,173,
  176,179,182,185,188,190,193,196,198,201,203,206,208,211,213,215,
  218,220,222,224,226,228,230,232,234,235,237,238,240,241,243,244,
  245,246,248,249,250,250,251,252,253,253,254,254,254,255,255,255,
  255,255,255,255,254,254,254,253,253,252,251,250,250,249,248,246,
  245,244,243,241,240,238,237,235,234,232,230,228,226,224,222,220,
  218,215,213,211,208,206,203,201,198,196,193,190,188,185,182,179,
  176,173,170,167,165,162,158,155,152,149,146,143,140,137,134,131,
  128,124,121,118,115,112,109,106,103,100,97,93,90,88,85,82,
  79,76,73,70,67,65,62,59,57,54,52,49,47,44,42,40,
  37,35,33,31,29,27,25,23,21,20,18,17,15,14,12,11,
  10,9,7,6,5,5,4,3,2,2,1,1,1,0,0,0,
  0,0,0,0,1,1,1,2,2,3,4,5,5,6,7,9,
  10,11,12,14,15,17,18,20,21,23,25,27,29,31,33,35,
  37,40,42,44,47,49,52,54,57,59,62,65,67,70,73,76,
  79,82,85,88,90,93,97,100,103,106,109,112,115,118,121,124
};*/
#endif /* DDS_TABLE_LARGE */

class DDS {
public:
  DDS(): refclk(DDS_REFCLK_DEFAULT), accumulator(0), running(false),
    timeLimited(false), tickDuration(0), amplitude(255)
    {};

  // Start all of the timers needed
  void start();
  // Is the DDS presently producing a tone?
  const bool isRunning() { return running; };
  // Stop the DDS timers
  void stop();
  
  // Start and stop the PWM output
  void on() {
    timeLimited = false;
    running = true;
  }
  // Provide a duration in ms for the tone
  void on(unsigned short duration) {
    // Duration in ticks from milliseconds is:
    // t = (1/refclk)
    tickDuration = (unsigned long)((unsigned long)duration * (unsigned long)refclk) / 1000;
    timeLimited = true;
    running = true;
  }
  void off() {
    running = false;
  }
  
  // Generate a tone for a specific amount of time
  void play(unsigned short freq, unsigned short duration) {
    setFrequency(freq);
    on(duration);
  }
  // Blocking version
  void playWait(unsigned short freq, unsigned short duration) {
    play(freq, duration);
    delay(duration);
  }
  
  // Use these to get some calculated values for specific frequencies
  // or to get the current frequency stepping rate.
  ddsAccumulator_t calcFrequency(unsigned short freq);
  ddsAccumulator_t getPhaseAdvance() { return stepRate; };
  
  // Our maximum clock isn't very high, so our highest
  // frequency supported will fit in a short.
  void setFrequency(unsigned short freq) { stepRate = calcFrequency(freq); };
  void setPrecalcFrequency(ddsAccumulator_t freq) { stepRate = freq; };
  
  // Handle phase shifts
  void setPhaseDeg(int16_t degrees);
  void changePhaseDeg(int16_t degrees);
  
  // Adjustable reference clock. This shoud be done before the timers are
  // started, or they will need to be restarted. Frequencies will need to
  // be set again to use the new clock.
  void setReferenceClock(unsigned long ref) {
    refclk = ref;
  }
  unsigned long getReferenceClock() {
    return refclk;
  }

  uint8_t getDutyCycle();

  // Set a scaling factor. To keep things quick, this is a power of 2 value.
  // Set it with 0 for lowest (which will be off), 8 is highest.
  void setAmplitude(unsigned char amp) {
    amplitude = amp;
  }
  
  // This is the function called by the ADC_vect ISR to produce the tones
  void clockTick();
  
private:
  volatile bool running;
  volatile unsigned long tickDuration;
  volatile bool timeLimited;
  volatile unsigned char amplitude;
  volatile ddsAccumulator_t accumulator;
  volatile ddsAccumulator_t stepRate;
  ddsAccumulator_t refclk;
  static DDS *sDDS;
};

#endif /* _DDS_H_ */
