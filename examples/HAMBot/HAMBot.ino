/* Simple DTMF controlled HAM Radio Robot */

#include <ArduinoRobot.h> // include the robot library
#include <HamShield.h>
#include <SPI.h>

#define PWM_PIN 3
#define RESET_PIN A3
#define SWITCH_PIN 2

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
  
  Robot.begin();

  radio.initialize();
  radio.frequency(145510);
}

void loop() {
   if(radio.waitForDTMF()) {                          // wait for a received DTMF tone
    uint8_t command = radio.getLastDTMFDigit();       // get the last DTMF tone sent
     if(command == '4') { Robot.turn(-90); }          // turn robot left
     if(command == '6') { Robot.turn(90); }           // turn robot right
     if(command == '2') { Robot.motorsWrite(-255,-255); delay(500); Robot.motorsWrite(255, 255); }  // move robot forward
     if(command == '5') {                             // tell robot to send morse code identity 
           if(radio.waitForChannel()) {               // wait for the user to release the transmit button
              radio.setModeTransmit();                // turn on transmit mode
              radio.morseOut("1ZZ9ZZ I AM HAMRADIO ROBOT");    // send morse code
              radio.setModeReceive();                 // go back to receive mode on radio 
           }
     }
   }
}

