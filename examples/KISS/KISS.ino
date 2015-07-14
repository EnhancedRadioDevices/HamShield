#include <HamShield.h>
#include <Wire.h>
#include <KISS.h>

HamShield radio;
DDS dds;
KISS kiss(&Serial, &radio, &dds);

void setup() {
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(11, INPUT);
  
  Serial.begin(9600);
  Wire.begin();
  radio.initialize();
  radio.setVHF();
  radio.setSQOff();
  radio.setFrequency(145010);
  I2Cdev::writeWord(A1846S_DEV_ADDR_SENLOW, 0x30, 0x06);
  I2Cdev::writeWord(A1846S_DEV_ADDR_SENLOW, 0x30, 0x26);
  I2Cdev::writeWord(A1846S_DEV_ADDR_SENLOW, 0x44, 0x05FF);

  dds.start();
  radio.afsk.start(&dds);
}

void loop() {
  kiss.loop();
}

ISR(ADC_vect) {
  kiss.isr();
}
