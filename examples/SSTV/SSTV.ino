/* 

Sends an SSTV test pattern 

*/

#define PWM_PIN 3
#define RESET_PIN A3
#define SWITCH_PIN 2

#define DOT 100
#define CALLSIGN "1ZZ9ZZ/B"

/* Standard libraries and variable init */

#include <HamShield.h>

HamShield radio;
int16_t rssi;

/* get our radio ready */

void setup() {
  // NOTE: if not using PWM out, it should be held low to avoid tx noise
  pinMode(PWM_PIN, OUTPUT);
  digitalWrite(PWM_PIN, LOW);
  
  // prep the switch
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  
  // set up the reset control pin
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, HIGH);
  
  Serial.begin(9600);
  Serial.print("Radio status: ");
  int result = radio.testConnection();
  Serial.println(result);
  radio.initialize();
  radio.frequency(446000);
  radio.setModeReceive();
}

/* main program loop */


void loop() {
     if(radio.waitForChannel(1000,2000)) {        // Wait forever for calling frequency to open, then wait 2 seconds for breakers 
       radio.setModeTransmit();                   // Turn on the transmitter
       delay(250);                               // Wait a moment
       radio.SSTVTestPattern(MARTIN1);            // send a MARTIN1 test pattern
       delay(250);
       radio.setModeReceive();                    // Turn off the transmitter
     } else { delay(30000); }                     // someone broke in fast after prior transmission, was it an emergency? wait 30 secs.
     
     delay(60000);                                // Wait a minute
}

