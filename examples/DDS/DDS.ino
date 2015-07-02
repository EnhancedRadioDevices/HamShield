#include <HamShield.h>
#include <Wire.h>

DDS dds;

void setup() {
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(11, OUTPUT);
  dds.start();
  dds.playWait(600, 3000);
  dds.on();
}

void loop() {
  dds.setFrequency(2200);
  delay(5000);
  dds.setFrequency(1200);
  delay(5000);
}

#ifdef DDS_USE_ONLY_TIMER2
ISR(TIMER2_OVF_vect) {
  dds.clockTick();
}
#else // Use the ADC timer instead
ISR(ADC_vect) {
  TIFR1 = _BV(ICF1); // Clear the timer flag
  dds.clockTick();
}
#endif
