// Example sketch to show how to transmit arbitrary tones.
// In this case, the sketch alternates between 1200Hz and 2200Hz at 1s intervals.

#define DDS_REFCLK_DEFAULT 9600
#include <HamShield.h>

#define PWM_PIN 3
#define RESET_PIN A3
#define SWITCH_PIN 2

HamShield radio;
DDS dds;

void setup() {
  // NOTE: if not using PWM out, it should be held low to avoid tx noise
  pinMode(PWM_PIN, OUTPUT);
  digitalWrite(PWM_PIN, LOW);
  
  // prep the switch
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  
  // set up the reset control pin
  pinMode(RESET_PIN, OUTPUT);
  // turn on radio
  digitalWrite(RESET_PIN, HIGH);
  
  radio.initialize();
  radio.setRfPower(0);
  radio.frequency(438000);
  radio.setModeTransmit();
  dds.start();
  dds.playWait(600, 3000);
  dds.on();
  //dds.setAmplitude(31);
}

void loop() {
  dds.setFrequency(2200);
  delay(1000);
  dds.setFrequency(1200);
  delay(1000);
}

#ifdef DDS_USE_ONLY_TIMER2
ISR(TIMER2_OVF_vect) {
  dds.clockTick();
}
#else // Use the ADC timer instead
ISR(ADC_vect) {
  static unsigned char tcnt = 0;
  TIFR1 = _BV(ICF1); // Clear the timer flag
  if(++tcnt == 4) {
    tcnt = 0;
  }
  dds.clockTick();
}
#endif
