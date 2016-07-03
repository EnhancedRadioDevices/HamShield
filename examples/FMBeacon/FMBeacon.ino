/* 
Morse Code Beacon

Test beacon will transmit and wait 30 seconds. 
Beacon will check to see if the channel is clear before it will transmit.

TO-DO: Radio chip audio AGC too slow in responding to tones, worked around by playing a 6khz tone between actual dits/dahs.
Should work on adjusting AGC to not require this.
*/

#define DDS_REFCLK_DEFAULT 9600
#include <HamShield.h>

#define PWM_PIN 3
#define RESET_PIN A3
#define SWITCH_PIN 2

HamShield radio;

// Run our start up things here
void setup() { 
  // NOTE: if not using PWM out, it should be held low to avoid tx noise
  pinMode(PWM_PIN, OUTPUT);
  digitalWrite(PWM_PIN, LOW);
  
  // prep the switch
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  
  // set up the reset control pin
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, HIGH);
  
  // Set up the serial port at 9600 Baud
  Serial.begin(9600);
  
  // Send a quick serial string
  Serial.println("HamShield FM Beacon Example Sketch");

  // Query the HamShield for status information
  Serial.print("Radio status: ");
  int result = radio.testConnection();
  Serial.println(result,DEC);
  
  // Tell the HamShield to start up
  radio.initialize();
  radio.setRfPower(0);

  // Configure the HamShield to transmit and recieve on 446.000MHz
  radio.frequency(438000);
  
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
    radio.morseOut(" KC7IBT ARDUINO HAMSHIELD");
    
    // We're done sending the message, set the radio back into recieve mode.
    radio.setModeReceive();
    Serial.println("Done.");
    
    // Wait a second before we send our beacon again.
    delay(1000);    
  } else {
    // If we get here, the channel is busy. Let's also print out the RSSI.
    Serial.print("The channel was busy. Waiting 10 seconds. RSSI: ");
    Serial.println(radio.readRSSI());
    
    // Wait 10 seconds and check the channel again.
    delay(10000);
  } 
}
