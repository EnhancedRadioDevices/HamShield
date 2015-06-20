/* Simple DTMF controlled HAM Radio Robot */

#include <ArduinoRobot.h> // include the robot library
#include <HAMShield.h>
#include <Wire.h>
#include <SPI.h>

HAMShield radio;

void setup() { 
  Robot.begin();
  Wire.begin();
  radio.initialize();
  radio.setFrequency(145510);
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

