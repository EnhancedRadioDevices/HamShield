/* Hamshield
 * Example: Fox Hunt
 * 
 * Plays a one minute tone, then IDs at 10-13 minute intervals. Script 
 * will check to see if the channel is clear before it will transmit.
 * 
 * Connect the HamShield to your Arduino. Screw the antenna 
 * into the HamShield RF jack. Connect the Arduino to wall 
 * power and then to your computer via USB. After uploading
 * this program to your Arduino, open the Serial Monitor to
 * monitor the status of the beacon. To test, set a HandyTalkie 
 * to 438MHz. You should hear a one-minute tone followed by
 * a callsign every 10-13 minutes.
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

  // Set up the serial port at 9600 Baud
  Serial.begin(9600);

  // Send a quick serial string
  Serial.println("HamShield FoxHunt Example Sketch");

  // Query the HamShield for status information
  Serial.print("Radio status: ");
  int result = radio.testConnection();
  Serial.println(result, DEC);

  // Tell the HamShield to start up
  radio.initialize();

  // Set the transmit power level (0-8)
  radio.setRfPower(0);

  // Set the morse code characteristics
  radio.setMorseFreq(600);
  radio.setMorseDotMillis(100);

  // Configure the HamShield to operate on 438.000Mhz
  radio.frequency(438000);

  Serial.println("Radio configured.");
}

void loop() {
  // We'll wait up to 30 seconds for a clear channel, requiring that the channel is clear for 2 seconds before we transmit
  if (radio.waitForChannel(30000,2000, -90)) {
    // If we get here, the channel is clear. Let's print the RSSI to the serial port as well.
    Serial.print("Signal is clear, RSSI: ");
    Serial.println(radio.readRSSI());

    // Set the HamShield to TX
    Serial.print("Transmitting...");
    radio.setModeTransmit();

    // Generate a 600Hz tone for TRANSMITLENGTH time
    tone(PWM_PIN, 600, TRANSMITLENGTH);
    delay(TRANSMITLENGTH);

    // Identify the transmitter
    radio.morseOut(" CALLSIGN FOXHUNT");

    // Set the HamShield back to RX
    radio.setModeReceive();
    Serial.println("Done.");

    // Wait for INTERLVAL + some random minutes before transmitting again
    waitMinute(INTERVAL + random(0,RANDOMCHANCE));
  }
}

// a function so we can wait by minutes
void waitMinute(unsigned long period) {
  Serial.print("Waiting for ");
  Serial.print(period, DEC);
  Serial.println(" minutes.");
  delay(period * 60 * 1000);
}
