#include <Arduino.h>
#include "DDS.h"

// To start the DDS, we use Timer1, set to the reference clock
// We use Timer2 for the PWM output, running as fast as feasible
void DDS::start() {
  // Use the clkIO clock rate
  ASSR &= ~(_BV(EXCLK) | _BV(AS2));
  
  // First, the timer for the PWM output
  // Setup the timer to use OC2B (pin 3) in fast PWM mode with a configurable top
  // Run it without the prescaler
#ifdef DDS_PWM_PIN_3
  TCCR2A = (TCCR2A | _BV(COM2B1)) & ~(_BV(COM2B0) | _BV(COM2A1) | _BV(COM2A0)) |
         _BV(WGM21) | _BV(WGM20);
#else
  // Alternatively, use pin 11
  TCCR2A = (TCCR2A | _BV(COM2A1)) & ~(_BV(COM2A0) | _BV(COM2B1) | _BV(COM2B0)) |
       _BV(WGM21) | _BV(WGM20);
#endif
  TCCR2B = (TCCR2B & ~(_BV(CS22) | _BV(CS21))) | _BV(CS20) | _BV(WGM22);

  // Set the top limit, which will be our duty cycle accuracy.
  // Setting Comparator Bits smaller will allow for higher frequency PWM,
  // with the loss of resolution.
  OCR2A = pow(2,COMPARATOR_BITS)-1;
  OCR2B = 0;
  
  // Second, setup Timer1 to trigger the ADC interrupt
  // This lets us use decoding functions that run at the same reference
  // clock as the DDS.
  TCCR1B = _BV(CS11) | _BV(WGM13) | _BV(WGM12);
  TCCR1A = 0;
  ICR1 = ((F_CPU / 8) / refclk) - 1;

  // Configure the ADC here to automatically run and be triggered off Timer1
  ADMUX = _BV(REFS0) | _BV(ADLAR) | 0; // Channel 0, shift result left (ADCH used)
  DDRC &= ~_BV(0);
  PORTC &= ~_BV(0);
  DIDR0 |= _BV(0);
  ADCSRB = _BV(ADTS2) | _BV(ADTS1) | _BV(ADTS0);
  ADCSRA = _BV(ADEN) | _BV(ADSC) | _BV(ADATE) | _BV(ADIE) | _BV(ADPS2); // | _BV(ADPS0);    
}

void DDS::stop() {
  // TODO: Stop the timers.
}

// Set our current sine wave frequency in Hz
void DDS::setFrequency(unsigned short freq) {
  // Fo = (M*Fc)/2^N
  // M = (Fo/Fc)*2^N
  if(refclk == DDS_REFCLK_DEAULT) {
    // Try to use precalculated values if possible
    if(freq == 2200) {
      stepRate = (2200.0 / DDS_REFCLK_DEAULT) * pow(2,ACCUMULATOR_BITS);
    } else if (freq == 1200) {
      stepRate = (1200.0 / DDS_REFCLK_DEAULT) * pow(2,ACCUMULATOR_BITS);      
    }
  } else {
    // Do the actual math instead.
    stepRate = (freq / refclk) * pow(2,ACCUMULATOR_BITS);    
  }
}

// TODO: Clean this up a bit..
void DDS::clockTick() {
  if(running) {
    accumulator += stepRate;
    if(timeLimited && tickDuration == 0) {
#ifdef DDS_IDLE_HIGH
      // Set the duty cycle to 50%
      OCR2B = pow(2,COMPARATOR_BITS)/2;
#else
      // Set duty cycle to 0, effectively off
      OCR2B = 0;
#endif
      running = false;
      accumulator = 0;
    } else {
      OCR2B = getDutyCycle();
    }
    // Reduce our playback duration by one tick
    tickDuration--;
  } else {
    // Hold it low
#ifdef DDS_IDLE_HIGH
    // Set the duty cycle to 50%
    OCR2B = pow(2,COMPARATOR_BITS)/2;
#else
    // Set duty cycle to 0, effectively off
    OCR2B = 0;
#endif
  }
}

uint8_t DDS::getDutyCycle() {
#if ACCUMULATOR_BIT_SHIFT >= 24
  uint16_t phAng;
#else
  uint8_t phAng;
#endif
  phAng = (accumulator >> ACCUMULATOR_BIT_SHIFT);
  uint8_t position = pgm_read_byte_near(ddsSineTable + phAng)>>(8-COMPARATOR_BITS);
  // Apply scaling and return
  return position >> amplitude;
}
