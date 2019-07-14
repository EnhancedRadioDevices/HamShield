/* Hamshield
 * Example: Morse Code Transceiver
 * 
 * Serial to Morse transceiver. Sends characters from the Serial
 * port over the air, and vice versa.
 * Connect the HamShield to your Arduino. Screw the antenna 
 * into the HamShield RF jack. Connect the Arduino to wall 
 * power and then to your computer via USB. After uploading 
 * this program to your Arduino, open the Serial Monitor to 
 * monitor the status of the beacon. To test, set a HandyTalkie 
 * to 438MHz. You should hear the message " CALLSIGN HAMSHIELD" 
 * in morse code.
 * 
 * 
 * Note: only upper case letters, numbers, and a few symbols
 * are supported.
 * Supported symbols: &/+(=:?";@`-._),!$
 * 
 * If you're having trouble accurately decoding, you may want to
 * tweak the min/max . and - times. You can also uncomment
 * the Serial.print debug statements that can tell you when tones
 * are being detected, how long they're detected for, and whether
 * the tones are decoded as a . or -.
 * 
*/

#define DDS_REFCLK_DEFAULT 9600
#include <HamShield.h>

#define MIC_PIN 3
#define RESET_PIN A3
#define SWITCH_PIN 2

#define MORSE_FREQ 600
#define MORSE_DOT 150 // ms
// Note that all timing is defined in terms of MORSE_DOT relative durations
// You may want to tweak those timings below


HamShield radio;

// Run our start up things here
void setup() { 
  // NOTE: if not using PWM out, it should be held low to avoid tx noise
  pinMode(MIC_PIN, OUTPUT);
  digitalWrite(MIC_PIN, LOW);
  
  // prep the switch
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  
  // set up the reset control pin
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, HIGH);
  delay(5); // wait for device to come up
  
  // Set up the serial port at 9600 Baud
  Serial.begin(9600);
  
  // Send a quick serial string
  Serial.println("HamShield Morse Example Sketch");
    
  Serial.print("Radio status: ");
  int result = radio.testConnection();
  Serial.println(result,DEC);

  // Tell the HamShield to start up
  radio.initialize();

  // Set the transmit power level (0-8)
  radio.setRfPower(0);

  // Set the morse code characteristics
  radio.setMorseFreq(MORSE_FREQ);
  radio.setMorseDotMillis(MORSE_DOT);
  
  radio.lookForTone(MORSE_FREQ);
  radio.setupMorseRx();
  
  // Configure the HamShield frequency
  radio.frequency(432100); // 70cm calling frequency
  radio.setModeReceive();
  
  Serial.println("Radio Configured.");
}

void loop() {
  char rx_char = radio.morseRxLoop();
  if (rx_char != 0) {
      Serial.print(rx_char);
  }
     
  // should we send anything
  if (Serial.available()) {
    Serial.println("checking channel");
    // We'll wait up to 30 seconds for a clear channel, requiring that the channel is clear for 2 seconds before we transmit
    if (radio.waitForChannel(30000,2000,-5)) {
      // If we get here, the channel is clear. 
      Serial.println("sending");

      // Start transmitting by putting the radio into transmit mode.
      radio.setModeTransmit();
      Serial.println("tx");
      unsigned int MORSE_BUF_SIZE = 128;
      char morse_buf[MORSE_BUF_SIZE];
      unsigned int morse_idx;
      morse_buf[morse_idx++] = ' '; // start with space to let PA come up
      while (Serial.available() && morse_idx < MORSE_BUF_SIZE) {  
        morse_buf[morse_idx++] = Serial.read();
      }
      morse_buf[morse_idx] = '\0'; // null terminate

      // Send a message out in morse code
      radio.morseOut(morse_buf);

      // We're done sending the message, set the radio back into recieve mode.
      Serial.println("sent");
      radio.setModeReceive();
      radio.lookForTone(MORSE_FREQ);
    } else {
      // If we get here, the channel is busy. Let's also print out the RSSI.
      Serial.print("The channel was busy. RSSI: ");
      Serial.println(radio.readRSSI());
    }
  }
}


