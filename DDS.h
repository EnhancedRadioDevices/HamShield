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
#define DDS_REFCLK_DEFAULT     9600
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
//#define DDS_DEBUG_SERIAL

// When defined, use the 1024 element sine lookup table. This improves phase
// accuracy, at the cost of more flash and CPU requirements.
// #define DDS_TABLE_LARGE

#ifdef DDS_TABLE_LARGE
// How many bits to keep from the accumulator to look up in this table
#define ACCUMULATOR_BIT_SHIFT (ACCUMULATOR_BITS-10)
static const int8_t ddsSineTable[1024] PROGMEM = {
  0, 0, 1, 2, 3, 3, 4, 5, 6, 7, 7, 8, 9, 10, 10, 11, 12, 13, 13, 14, 15, 16, 17, 17, 18, 19, 20, 20, 21, 22, 23, 24, 
  24, 25, 26, 27, 27, 28, 29, 30, 30, 31, 32, 33, 33, 34, 35, 36, 36, 37, 38, 39, 39, 40, 41, 42, 42, 43, 44, 44, 45, 46, 47, 47, 
  48, 49, 50, 50, 51, 52, 52, 53, 54, 55, 55, 56, 57, 57, 58, 59, 59, 60, 61, 61, 62, 63, 63, 64, 65, 65, 66, 67, 67, 68, 69, 69, 
  70, 71, 71, 72, 73, 73, 74, 75, 75, 76, 76, 77, 78, 78, 79, 79, 80, 81, 81, 82, 82, 83, 84, 84, 85, 85, 86, 87, 87, 88, 88, 89, 
  89, 90, 90, 91, 91, 92, 93, 93, 94, 94, 95, 95, 96, 96, 97, 97, 98, 98, 99, 99, 100, 100, 101, 101, 102, 102, 102, 103, 103, 104, 104, 105, 
  105, 106, 106, 106, 107, 107, 108, 108, 108, 109, 109, 110, 110, 110, 111, 111, 112, 112, 112, 113, 113, 113, 114, 114, 114, 115, 115, 115, 116, 116, 116, 117, 
  117, 117, 117, 118, 118, 118, 119, 119, 119, 119, 120, 120, 120, 120, 121, 121, 121, 121, 121, 122, 122, 122, 122, 123, 123, 123, 123, 123, 123, 124, 124, 124, 
  124, 124, 124, 124, 125, 125, 125, 125, 125, 125, 125, 125, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 
  127, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 125, 125, 125, 125, 125, 125, 125, 125, 124, 124, 124, 
  124, 124, 124, 124, 123, 123, 123, 123, 123, 123, 122, 122, 122, 122, 121, 121, 121, 121, 121, 120, 120, 120, 120, 119, 119, 119, 119, 118, 118, 118, 117, 117, 
  117, 117, 116, 116, 116, 115, 115, 115, 114, 114, 114, 113, 113, 113, 112, 112, 112, 111, 111, 110, 110, 110, 109, 109, 108, 108, 108, 107, 107, 106, 106, 106, 
  105, 105, 104, 104, 103, 103, 102, 102, 102, 101, 101, 100, 100, 99, 99, 98, 98, 97, 97, 96, 96, 95, 95, 94, 94, 93, 93, 92, 91, 91, 90, 90, 
  89, 89, 88, 88, 87, 87, 86, 85, 85, 84, 84, 83, 82, 82, 81, 81, 80, 79, 79, 78, 78, 77, 76, 76, 75, 75, 74, 73, 73, 72, 71, 71, 
  70, 69, 69, 68, 67, 67, 66, 65, 65, 64, 63, 63, 62, 61, 61, 60, 59, 59, 58, 57, 57, 56, 55, 55, 54, 53, 52, 52, 51, 50, 50, 49, 
  48, 47, 47, 46, 45, 44, 44, 43, 42, 42, 41, 40, 39, 39, 38, 37, 36, 36, 35, 34, 33, 33, 32, 31, 30, 30, 29, 28, 27, 27, 26, 25, 
  24, 24, 23, 22, 21, 20, 20, 19, 18, 17, 17, 16, 15, 14, 13, 13, 12, 11, 10, 10, 9, 8, 7, 7, 6, 5, 4, 3, 3, 2, 1, 0, 
  0, 0, -1, -2, -3, -3, -4, -5, -6, -7, -7, -8, -9, -10, -10, -11, -12, -13, -13, -14, -15, -16, -17, -17, -18, -19, -20, -20, -21, -22, -23, -24, 
  -24, -25, -26, -27, -27, -28, -29, -30, -30, -31, -32, -33, -33, -34, -35, -36, -36, -37, -38, -39, -39, -40, -41, -42, -42, -43, -44, -44, -45, -46, -47, -47, 
  -48, -49, -50, -50, -51, -52, -52, -53, -54, -55, -55, -56, -57, -57, -58, -59, -59, -60, -61, -61, -62, -63, -63, -64, -65, -65, -66, -67, -67, -68, -69, -69, 
  -70, -71, -71, -72, -73, -73, -74, -75, -75, -76, -76, -77, -78, -78, -79, -79, -80, -81, -81, -82, -82, -83, -84, -84, -85, -85, -86, -87, -87, -88, -88, -89, 
  -89, -90, -90, -91, -91, -92, -93, -93, -94, -94, -95, -95, -96, -96, -97, -97, -98, -98, -99, -99, -100, -100, -101, -101, -102, -102, -102, -103, -103, -104, -104, -105, 
  -105, -106, -106, -106, -107, -107, -108, -108, -108, -109, -109, -110, -110, -110, -111, -111, -112, -112, -112, -113, -113, -113, -114, -114, -114, -115, -115, -115, -116, -116, -116, -117, 
  -117, -117, -117, -118, -118, -118, -119, -119, -119, -119, -120, -120, -120, -120, -121, -121, -121, -121, -121, -122, -122, -122, -122, -123, -123, -123, -123, -123, -123, -124, -124, -124, 
  -124, -124, -124, -124, -125, -125, -125, -125, -125, -125, -125, -125, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, 
  -127, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -125, -125, -125, -125, -125, -125, -125, -125, -124, -124, -124, 
  -124, -124, -124, -124, -123, -123, -123, -123, -123, -123, -122, -122, -122, -122, -121, -121, -121, -121, -121, -120, -120, -120, -120, -119, -119, -119, -119, -118, -118, -118, -117, -117, 
  -117, -117, -116, -116, -116, -115, -115, -115, -114, -114, -114, -113, -113, -113, -112, -112, -112, -111, -111, -110, -110, -110, -109, -109, -108, -108, -108, -107, -107, -106, -106, -106, 
  -105, -105, -104, -104, -103, -103, -102, -102, -102, -101, -101, -100, -100, -99, -99, -98, -98, -97, -97, -96, -96, -95, -95, -94, -94, -93, -93, -92, -91, -91, -90, -90, 
  -89, -89, -88, -88, -87, -87, -86, -85, -85, -84, -84, -83, -82, -82, -81, -81, -80, -79, -79, -78, -78, -77, -76, -76, -75, -75, -74, -73, -73, -72, -71, -71, 
  -70, -69, -69, -68, -67, -67, -66, -65, -65, -64, -63, -63, -62, -61, -61, -60, -59, -59, -58, -57, -57, -56, -55, -55, -54, -53, -52, -52, -51, -50, -50, -49, 
  -48, -47, -47, -46, -45, -44, -44, -43, -42, -42, -41, -40, -39, -39, -38, -37, -36, -36, -35, -34, -33, -33, -32, -31, -30, -30, -29, -28, -27, -27, -26, -25, 
  -24, -24, -23, -22, -21, -20, -20, -19, -18, -17, -17, -16, -15, -14, -13, -13, -12, -11, -10, -10, -9, -8, -7, -7, -6, -5, -4, -3, -3, -2, -1, 0
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
#endif /* DDS_TABLE_LARGE */

class DDS {
public:
  DDS(): refclk(DDS_REFCLK_DEFAULT), refclkOffset(DDS_REFCLK_OFFSET),
    accumulator(0), running(false),
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

  void setReferenceOffset(int16_t offset) {
    refclkOffset = offset;
  }
  int16_t getReferenceOffset() {
    return refclkOffset;
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
  int16_t refclkOffset;
  static DDS *sDDS;
};

#endif /* _DDS_H_ */
