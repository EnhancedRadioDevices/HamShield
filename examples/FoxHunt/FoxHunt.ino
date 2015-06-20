/* Fox Hunt */

#include <HAMShield.h>
#include <Wire.h>

// transmit for 1 minute, every 10 minutes 

#define TRANSMITLENGTH 1
#define INTERVAL 10
#define RANDOMCHANCE 3

HAMShield radio;

void setup() { 
  Wire.begin();
  radio.initialize();
  radio.setFrequency(145510);
  radio.setModeReceive();
}

void loop() {
   waitMinute(INTERVAL + random(0,RANDOMCHANCE));     // wait before transmitting, randomly up to 3 minutes later
   if(radio.waitForChannel(30000,2000)) {             // wait for a clear channel, abort after 30 seconds, wait 2 seconds of dead air for breakers
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


