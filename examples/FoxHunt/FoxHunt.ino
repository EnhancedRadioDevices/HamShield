/* Hamshield
 * Example: Fox Hunt
 * Plays a one minute tone at 10-13 minute intervals. Script 
 * will check to see if the channel is clear before it will 
 * transmit.
 * Connect the HamShield to your Arduino. Screw the antenna 
 * into the HamShield RF jack. Connect the Arduino to wall 
 * power and then to your computer via USB. To test, set a 
 * HandyTalkie to 438MHz. You should hear a one-minute tone 
 * followed by a callsign every 10-13 minutes.
*/

#include <HamShield.h>

#define PWM_PIN 3
#define RESET_PIN A3
#define SWITCH_PIN 2

// In milliseconds
#define TRANSMITLENGTH 60000
// In minutes
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
  
  radio.initialize();
  radio.setRfPower(0);
  radio.frequency(438000);
  radio.setModeReceive();
}

void loop() {
  if(radio.waitForChannel(30000,2000, -90)) {             // wait for a clear channel, abort after 30 seconds, wait 2 seconds of dead air for breakers
    radio.setModeTransmit();                         // turn on transmit mode
    tone(PWM_PIN, 1000, TRANSMITLENGTH);        // play a long solid tone
    delay(TRANSMITLENGTH);
    radio.morseOut(" 1ZZ9ZZ/B FOXHUNT");              // identify the fox hunt transmitter
    radio.setModeReceive();                          // turn off the transmit mode
  }
  waitMinute(INTERVAL + random(0,RANDOMCHANCE));     // wait before transmitting, randomly
}

// a function so we can wait by minutes
void waitMinute(unsigned long period) { 
  delay(period * 60 * 1000);
}
