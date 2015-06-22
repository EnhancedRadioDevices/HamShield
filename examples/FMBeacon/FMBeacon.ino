/* 

Morse Code Beacon

Test beacon will transmit and wait 30 seconds. 
Beacon will check to see if the channel is clear before it will transmit.


*/

#include <HamShield.h>
#include <Wire.h>

HamShield radio;

void setup() { 
  Serial.begin(9600);
  Serial.println("starting up..");
  Wire.begin();

  Serial.print("Radio status: ");
  int result = radio.testConnection();
  Serial.println(result,DEC);
  radio.initialize();                         // setup radio
  radio.frequency(446000);                 // set to 70 cm call frequency
  Serial.println("Done with radio beacon setup.");
}

void loop() {
  if(radio.waitForChannel(30000,2000,-50)) {     // wait up to 30 seconds for a clear channel, and then 2 seconds of empty channel
    Serial.println("Signal is clear -- Transmitting");
    radio.setModeTransmit();                // turn on the transmitter
    radio.morseOut("1ZZ9ZZ/B CN87 ARDUINO HAMSHIELD");
    radio.setModeReceive();                 // turn off the transmitter (receive mode)
    Serial.print("TX Off");
    delay(30000);    
  } else { 
    Serial.print("The channel was busy. Waiting 10 seconds. RSSI: ");
    Serial.println(radio.readRSSI());
    delay(10000);
  } 
}


