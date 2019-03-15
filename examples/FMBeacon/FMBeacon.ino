/* Hamshield
 * Example: Morse Code Beacon
 * 
 * Test beacon will transmit and wait 30 seconds. 
 * Beacon will check to see if the channel is clear before it 
 * will transmit.
 * Connect the HamShield to your Arduino. Screw the antenna 
 * into the HamShield RF jack. Connect the Arduino to wall 
 * power and then to your computer via USB. After uploading 
 * this program to your Arduino, open the Serial Monitor to 
 * monitor the status of the beacon. To test, set a HandyTalkie 
 * to 438MHz. You should hear the message " CALLSIGN HAMSHIELD" 
 * in morse code.
*/

#define DDS_REFCLK_DEFAULT 9600
#include <HamShield.h>

#define MIC_PIN 3
#define RESET_PIN A3
#define SWITCH_PIN 2

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
  Serial.println("HamShield FM Beacon Example Sketch");

  // Query the HamShield for status information
  Serial.print("Radio status: ");
  int result = radio.testConnection();
  Serial.println(result,DEC);
  
  // Tell the HamShield to start up
  radio.initialize();

  // Set the transmit power level (0-8)
  radio.setRfPower(0);

  // Set the morse code characteristics
  radio.setMorseFreq(600);
  radio.setMorseDotMillis(100);

  // Configure the HamShield to operate on 438.000MHz
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
    radio.morseOut(" CALLSIGN HAMSHIELD");
    
    // We're done sending the message, set the radio back into recieve mode.
    radio.setModeReceive();
    Serial.println("Done.");
  } else {
    // If we get here, the channel is busy. Let's also print out the RSSI.
    Serial.print("The channel was busy. Waiting 10 seconds. RSSI: ");
    Serial.println(radio.readRSSI());
  }

  // Wait 30 seconds before we send our beacon again.
  delay(30000);
}
