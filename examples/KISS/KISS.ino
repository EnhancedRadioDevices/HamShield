#include <HamShield.h>
#include <KISS.h>

HamShield radio;
DDS dds;
KISS kiss(&Serial, &radio, &dds);

//TODO: move these into library
#define PWM_PIN 3
#define RESET_PIN A3
#define SWITCH_PIN 2

void setup() {
  // NOTE: if not using PWM out, it should be held low to avoid tx noise
  pinMode(PWM_PIN, OUTPUT);
  digitalWrite(PWM_PIN, LOW);
  
  // prep the switch
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  
  // set up the reset control pin
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, LOW);
  
  Serial.begin(9600);
  
  while (digitalRead(SWITCH_PIN));
  
  // let the AU ot of reset
  digitalWrite(RESET_PIN, HIGH);
  
  radio.initialize();
  radio.setSQOff();
  radio.frequency(144390);
  //I2Cdev::writeWord(A1846S_DEV_ADDR_SENLOW, 0x44, 0x05FF);

  dds.start();
  radio.afsk.start(&dds);
}

void loop() {
  kiss.loop();
}

ISR(ADC_vect) {
  kiss.isr();
}
