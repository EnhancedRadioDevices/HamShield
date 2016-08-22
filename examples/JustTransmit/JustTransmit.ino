/* Hamshield
 * Example: Just Transmit
 * This example continuously transmits.
 * Connect the HamShield to your Arduino. Screw the antenna 
 * into the HamShield RF jack. Plug a pair of headphones with 
 * built-in mic into the HamShield. Connect the Arduino to 
 * wall power and then to your computer via USB. After 
 * uploading this program to your adruino, open the Serial 
 * Monitor to monitor the program's progress. After setup is 
 * complete, tune a HandyTalkie (HT) to 144.025MHz. Listen on 
 * the HT for the HamShield broadcasting from the mic.
*/

#include <HamShield.h>

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
  
  Serial.begin(9600);
  Serial.println("If the sketch freezes at radio status, there is something wrong with power or the shield");
  Serial.print("Radio status: ");
  int result = radio.testConnection();
  Serial.println(result,DEC);
  Serial.println("Setting radio to its defaults..");
  radio.initialize();
  radio.setRfPower(0);
}

void loop() {
  radio.bypassPreDeEmph();
  radio.frequency(144025);
  // radio.setTxSourceNone();
  radio.setModeTransmit();
  for(;;) { }
}

