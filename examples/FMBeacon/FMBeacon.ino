/* 
Morse Code Beacon

Test beacon will transmit and wait 30 seconds. 
Beacon will check to see if the channel is clear before it will transmit.
*/

// Include the HamSheild and Wire (I2C) libraries
#include <HamShield.h>
#include <Wire.h>

// Create a new instance of our HamSheild class, called 'radio'
HamShield radio;

// Run our start up things here
void setup() { 
  // Set up the serial port at 9600 Baud
  Serial.begin(9600);
  
  // Send a quick serial string
  Serial.println("HamShield FM Beacon Example Sketch");
  
  // Start the Wire (I2C) library
  Wire.begin();

  // Query the HamShield for status information
  Serial.print("Radio status: ");
  int result = radio.testConnection();
  Serial.println(result,DEC);
  
  // Tell the HamShield to start up
  radio.initialize();
  radio.setRfPower(15);
  // Configure the HamShield to transmit and recieve on 446.000MHz
  radio.frequency(145570);
  pinMode(11, INPUT); // Bodge for now, as pin 3 is hotwired to pin 11
  
  Serial.println("Radio Configured.");
}

void loop() {
  // We'll wait up to 30 seconds for a clear channel, requiring that the channel is clear for 2 seconds before we transmit
  if (radio.waitForChannel(30000,2000,-5)) {
    // If we get here, the channel is clear. Let's print the RSSI to the serial port as well.
    Serial.print("Signal is clear, RSSI: ");
    Serial.println(radio.readRSSI());
    
    // Start transmitting by putting the radio into transmit mode.
    Serial.print("Transmitting... ");
    radio.setModeTransmit();
    
    // Send a message out in morse code
    radio.morseOut("KC7IBT ARDUINO HAMSHIELD");
    
    // We're done sending the message, set the radio back into recieve mode.
    radio.setModeReceive();
    Serial.println("Done.");
    
    // Wait 30 seconds before we send our beacon again.
    delay(1000);    
  } else {
    // If we get here, the channel is busy. Let's also print out the RSSI.
    Serial.print("The channel was busy. Waiting 10 seconds. RSSI: ");
    Serial.println(radio.readRSSI());
    
    // Wait 10 seconds and check the channel again.
    delay(10000);
  } 
}
