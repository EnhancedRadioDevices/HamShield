/* Hamshield
 * Example: KISS
 * This is a example configures the HamShield to be used as 
 * a TNC/KISS device. You will need a KISS device to input 
 * commands to the HamShield
 * Connect the HamShield to your Arduino. Screw the antenna 
 * into the HamShield RF jack. Connect the Arduino to wall 
 * power and then to your computer via USB. Issue commands 
 * via the KISS equipment.
*/

#include <HamShield.h>
#include <KISS.h>
#include <packet.h>

HamShield radio;
DDS dds;
KISS kiss(&Serial, &radio, &dds);
AFSK afsk;

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

  dds.start();
  afsk.start(&dds);
}

void loop() {
  kiss.loop();
}

ISR(ADC_vect) {
  kiss.isr();
}
