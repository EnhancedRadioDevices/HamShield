#include <Arduino.h>
#include "DDS.h"

#ifdef __SAMD21G18A__

// The SimpleAudioPlayerZero sample project found at:
// https://www.arduino.cc/en/Tutorial/SimpleAudioPlayerZero
// is an execellent reference for setting up the Timer/Counter
#define TC_ISBUSY()   (TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY)
#define TC_WAIT()     while (TC_ISBUSY());
#define TC_ENABLE()   TC5->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE; TC_WAIT();                    
#define TC_RESET()    TC5->COUNT16.CTRLA.reg = TC_CTRLA_SWRST; TC_WAIT(); \
                      while (TC5->COUNT16.CTRLA.bit.SWRST);
#define TC_DISABLE()  TC5->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE; TC_WAIT();

#endif

// To start the DDS, we use Timer1, set to the reference clock
// We use Timer2 for the PWM output, running as fast as feasible
void DDS::start() {

#ifdef DDS_DEBUG_SERIAL
  // Print these debug statements (commont to both the AVR and the SAMD21)
  Serial.print(F("DDS SysClk: "));
  Serial.println(F_CPU/8);
  Serial.print(F("DDS RefClk: "));
  Serial.println(refclk, DEC);
#endif

#ifdef __SAMD21G18A__
  // Enable the Generic Clock Generator 0 and configure for TC4 and TC5.
  // We only need TC5, but they are configured together
  GCLK->CLKCTRL.reg = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID(GCM_TC4_TC5)) ;
  while (GCLK->STATUS.bit.SYNCBUSY);

  TC_RESET();

  // Set TC5 16 bit
  TC5->COUNT16.CTRLA.reg |= TC_CTRLA_MODE_COUNT16;

  // Set TC5 mode as match frequency
  TC5->COUNT16.CTRLA.reg |= TC_CTRLA_WAVEGEN_MFRQ;
  TC5->COUNT16.CTRLA.reg |= TC_CTRLA_PRESCALER_DIV1 | TC_CTRLA_ENABLE;
  TC5->COUNT16.CC[0].reg = (uint16_t) (SystemCoreClock / DDS_REFCLK_DEFAULT - 1);
  TC_WAIT()

  // Configure interrupt
  NVIC_DisableIRQ(TC5_IRQn);
  NVIC_ClearPendingIRQ(TC5_IRQn);
  NVIC_SetPriority(TC5_IRQn, 0);
  NVIC_EnableIRQ(TC5_IRQn);

  // Enable TC5 Interrupt
  TC5->COUNT16.INTENSET.bit.MC0 = 1;
  TC_WAIT();

  //Configure the DAC
  analogWriteResolution(8);
  analogWrite(A0, 0);
#else
  // Use the clkIO clock rate
  ASSR &= ~(_BV(EXCLK) | _BV(AS2));
  
  // First, the timer for the PWM output
  // Setup the timer to use OC2B (pin 3) in fast PWM mode with a configurable top
  // Run it without the prescaler
  #ifdef DDS_PWM_PIN_3
    TCCR2A = (TCCR2A | _BV(COM2B1)) & ~(_BV(COM2B0) | _BV(COM2A1) | _BV(COM2A0)) |
         _BV(WGM21) | _BV(WGM20);
    TCCR2B = (TCCR2B & ~(_BV(CS22) | _BV(CS21))) | _BV(CS20) | _BV(WGM22);
  #else
  // Alternatively, use pin 11
  // Enable output compare on OC2A, toggle mode
    TCCR2A = _BV(COM2A1) | _BV(WGM21) | _BV(WGM20);
    //TCCR2A = (TCCR2A | _BV(COM2A1)) & ~(_BV(COM2A0) | _BV(COM2B1) | _BV(COM2B0)) |
    //     _BV(WGM21) | _BV(WGM20);
    TCCR2B = _BV(CS20);
  #endif

  // Set the top limit, which will be our duty cycle accuracy.
  // Setting Comparator Bits smaller will allow for higher frequency PWM,
  // with the loss of resolution.
  #ifdef DDS_PWM_PIN_3
    OCR2A = pow(2,COMPARATOR_BITS)-1;
    OCR2B = 0;
  #else
    OCR2A = 0;
  #endif
  
  #ifdef DDS_USE_ONLY_TIMER2
    TIMSK2 |= _BV(TOIE2);
  #endif

  // Second, setup Timer1 to trigger the ADC interrupt
  // This lets us use decoding functions that run at the same reference
  // clock as the DDS.
  // We use ICR1 as TOP and prescale by 8
  TCCR1B = _BV(CS10) | _BV(WGM13) | _BV(WGM12);
  TCCR1A = 0;
  ICR1 = ((F_CPU / 1) / refclk) - 1; 
  #ifdef DDS_DEBUG_SERIAL
    Serial.print(F("DDS ICR1: "));
    Serial.println(ICR1, DEC);
  #endif

  // Configure the ADC here to automatically run and be triggered off Timer1
  ADMUX = _BV(REFS0) | _BV(ADLAR) | 0; // Channel 0, shift result left (ADCH used)
  DDRC &= ~_BV(0);
  PORTC &= ~_BV(0);
  DIDR0 |= _BV(0);
  ADCSRB = _BV(ADTS2) | _BV(ADTS1) | _BV(ADTS0);
  ADCSRA = _BV(ADEN) | _BV(ADSC) | _BV(ADATE) | _BV(ADIE) | _BV(ADPS2); // | _BV(ADPS0);    

#endif
}

void DDS::stop() {
#ifdef __SAMD21G18A__
  TC_DISABLE();
  TC_RESET();
  analogWrite(A0, 0);
#else
  // TODO: Stop the timers.
  #ifndef DDS_USE_ONLY_TIMER2
    TCCR1B = 0;
  #endif
    TCCR2B = 0;
#endif
}

// Set our current sine wave frequency in Hz
ddsAccumulator_t DDS::calcFrequency(unsigned short freq) {
  // Fo = (M*Fc)/2^N
  // M = (Fo/Fc)*2^N
  ddsAccumulator_t newStep;
  if(refclk == DDS_REFCLK_DEFAULT) {
    // Try to use precalculated values if possible
    if(freq == 2200) {
      newStep = (2200.0 / (DDS_REFCLK_DEFAULT+DDS_REFCLK_OFFSET)) * pow(2,ACCUMULATOR_BITS);
    } else if (freq == 1200) {
      newStep = (1200.0 / (DDS_REFCLK_DEFAULT+DDS_REFCLK_OFFSET)) * pow(2,ACCUMULATOR_BITS);      
    } else if(freq == 2400) {
      newStep = (2400.0 / (DDS_REFCLK_DEFAULT+DDS_REFCLK_OFFSET)) * pow(2,ACCUMULATOR_BITS);
    } else if (freq == 1500) {
      newStep = (1500.0 / (DDS_REFCLK_DEFAULT+DDS_REFCLK_OFFSET)) * pow(2,ACCUMULATOR_BITS);      
    } else if (freq == 600) {
      newStep = (600.0 / (DDS_REFCLK_DEFAULT+DDS_REFCLK_OFFSET)) * pow(2,ACCUMULATOR_BITS);      
    }
  } else {
    //TODO:  This doesn't work with the SAM21D... yet
    newStep = pow(2,ACCUMULATOR_BITS)*freq / (refclk+refclkOffset);
  }
  return newStep;
}

// Degrees should be between -360 and +360 (others don't make much sense)
void DDS::setPhaseDeg(int16_t degrees) {
  accumulator = degrees * (pow(2,ACCUMULATOR_BITS)/360.0);
}
void DDS::changePhaseDeg(int16_t degrees) {
  accumulator += degrees * (pow(2,ACCUMULATOR_BITS)/360.0);
}

// TODO: Clean this up a bit..
void DDS::clockTick() {
/*  if(running) {
    accumulator += stepRate;
    OCR2A = getDutyCycle();
  }
  return;*/
  if(running) {
    accumulator += stepRate;
    if(timeLimited && tickDuration == 0) {
#ifdef __SAMD21G18A__
  #ifdef DDS_IDLE_HIGH
    analogWrite(A0, pow(2,COMPARATOR_BITS)/2);
  #else
    analogWrite(A0, 0);
  #endif
#else
  #ifndef DDS_PWM_PIN_3
      OCR2A = 0;
  #else
  #ifdef DDS_IDLE_HIGH
      // Set the duty cycle to 50%
      OCR2B = pow(2,COMPARATOR_BITS)/2;
  #else
      // Set duty cycle to 0, effectively off
        OCR2B = 0;
  #endif
  #endif
#endif
      running = false;
      accumulator = 0;
    } else {
#ifdef __SAMD21G18A__
    analogWrite(A0, getDutyCycle());
#else
  #ifdef DDS_PWM_PIN_3
      OCR2B = getDutyCycle();
  #else
      OCR2A = getDutyCycle();
  #endif
#endif
    }
    // Reduce our playback duration by one tick
    tickDuration--;
  } else {
#ifdef __SAMD21G18A__
  #ifdef DDS_IDLE_HIGH
    analogWrite(A0, pow(2,COMPARATOR_BITS)/2);
  #else
    analogWrite(A0, 0);
  #endif
#else
    // Hold it low
  #ifndef DDS_PWM_PIN_3
    OCR2A = 0;
  #else
    #ifdef DDS_IDLE_HIGH
    // Set the duty cycle to 50%
      OCR2B = pow(2,COMPARATOR_BITS)/2;
    #else
      // Set duty cycle to 0, effectively off
      OCR2B = 0;
    #endif
  #endif
#endif
  }
}

ddsComparitor_t DDS::getDutyCycle() {
  #if ACCUMULATOR_BIT_SHIFT >= 24
    uint16_t phAng;
  #else
    uint8_t phAng;
  #endif
  if(amplitude == 0) // Shortcut out on no amplitude
    return 128>>(8-COMPARATOR_BITS);
  phAng = (accumulator >> ACCUMULATOR_BIT_SHIFT);
  int8_t position = pgm_read_byte_near(ddsSineTable + phAng); //>>(8-COMPARATOR_BITS);
  // Apply scaling and return
  int16_t scaled = position;
  // output = ((duty * amplitude) / 256) + 128
  // This keeps amplitudes centered around 50% duty
  if(amplitude != 255) { // Amplitude is reduced, so do the full math
    scaled *= amplitude;
    scaled >>= 8+(8-COMPARATOR_BITS);
  } else { // Otherwise, only shift for the comparator bits
    scaled >>= (8-COMPARATOR_BITS);
  }
  scaled += 128>>(8-COMPARATOR_BITS);
  return scaled;
}

