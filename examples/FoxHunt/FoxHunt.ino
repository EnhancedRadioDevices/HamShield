/* Fox Hunt */

#include <HamShield.h>
//#include <Wire.h>

#define PWM_PIN 3
#define RESET_PIN A3
#define SWITCH_PIN 2

// transmit for 1 minute, every 10 minutes 

#define TRANSMITLENGTH 1
#define INTERVAL 10
#define RANDOMCHANCE 3

HamShield radio;

void setup() {
  // NOTE: if not using PWM out, it should be held low to avoid tx noise
  pinMode(PWM_PIN, OUTPUT);
  digitalWrite(PWM_PIN, LOW);
  
  // prep the switch
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  
  // set up the reset control pin
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, HIGH);
  
  //Wire.begin();
  radio.initialize();
  radio.frequency(145510);
  radio.setModeReceive();
}

void loop() {
   waitMinute(INTERVAL + random(0,RANDOMCHANCE));     // wait before transmitting, randomly up to 3 minutes later
   if(radio.waitForChannel(30000,2000, -90)) {             // wait for a clear channel, abort after 30 seconds, wait 2 seconds of dead air for breakers
     radio.setModeTransmit();                         // turn on transmit mode
     tone(1000,11,TRANSMITLENGTH * 60 * 1000);        // play a long solid tone
     radio.morseOut("1ZZ9ZZ/B FOXHUNT");              // identify the fox hunt transmitter
     radio.setModeReceive();                          // turn off the transmit mode
   }
}

// a function so we can wait by minutes

void waitMinute(int period) { 
  delay(period * 60 * 1000);
}

