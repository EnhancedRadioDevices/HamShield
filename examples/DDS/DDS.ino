#define DDS_REFCLK_DEFAULT 9600
#include <HamShield.h>
#include <Wire.h>

HamShield radio;
DDS dds;

void setup() {
  Wire.begin();
  radio.initialize();
  radio.setRfPower(0);
  radio.setVHF();
  radio.setFrequency(145060);
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(11, INPUT);
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
  TIFR1 = _BV(ICF1); // Clear the timer flag
  dds.clockTick();
}
#endif
