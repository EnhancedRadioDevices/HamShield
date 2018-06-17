/* Hamshield
 * Example: HandyTalkie
 * This is a simple example to demonstrate HamShield receive
 * and transmit functionality.
 * Connect the HamShield to your Arduino. Screw the antenna 
 * into the HamShield RF jack. Plug a pair of headphones into 
 * the HamShield. Connect the Arduino to wall power and then 
 * to your computer via USB. After uploading this program to 
 * your Arduino, open the Serial Monitor. Press the button on 
 * the HamShield to begin setup. After setup is complete, type 
 * your desired Tx/Rx frequency, in hertz, into the bar at the 
 * top of the Serial Monitor and click the "Send" button. 
 * To test with another HandyTalkie (HT), key up on your HT 
 * and make sure you can hear it through the headphones 
 * attached to the HamShield. Key up on the HamShield by 
 * holding the button.
*/

#include <HamShield.h>

// create object for radio
HamShield radio;

#define LED_PIN 13

#define PWM_PIN 3
#define RESET_PIN A3
#define SWITCH_PIN 2

uint32_t freq;

void setup() {
  // NOTE: if not using PWM out, it should be held low to avoid tx noise
  pinMode(PWM_PIN, OUTPUT);
  digitalWrite(PWM_PIN, LOW);
  
  // prep the switch
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  
  // set up the reset control pin
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, LOW);
  
  
  // initialize serial communication
  Serial.begin(9600);
  Serial.println("press the switch to begin...");
  
  while (digitalRead(SWITCH_PIN));
  
  // let the AU ot of reset
  digitalWrite(RESET_PIN, HIGH);
  
  Serial.println("beginning radio setup");

  // verify connection
  Serial.println("Testing device connections...");
  Serial.println(radio.testConnection() ? "RDA radio connection successful" : "RDA radio connection failed");

  // initialize device
  Serial.println("Initializing I2C devices...");
  radio.initialize(); // initializes automatically for UHF 12.5kHz channel

  Serial.println("setting default Radio configuration");
  radio.dangerMode();

  // set frequency
  Serial.println("setting squelch");

  radio.setSQHiThresh(-10);
  radio.setSQLoThresh(-30);
  Serial.print("sq hi: ");
  Serial.println(radio.getSQHiThresh());
  Serial.print("sq lo: ");
  Serial.println(radio.getSQLoThresh());
  radio.setSQOn();
  //radio.setSQOff();

  // set frequency
  Serial.println("changing frequency");
  freq = 415000;
  radio.frequency(freq);
  
  // set RX volume to minimum to reduce false positives on DTMF rx
  radio.setVolume1(0);
  radio.setVolume2(0);
  
  // set to receive
  radio.setModeReceive();
  Serial.print("config register is: ");
  Serial.println(radio.readCtlReg());
  Serial.println(radio.readRSSI());
  
  radio.setRfPower(0);
    
  // configure Arduino LED for
  pinMode(LED_PIN, OUTPUT);

  // set up DTMF
  radio.enableDTMFReceive(); // enabled DTMF
  Serial.println("ready");
}

void loop() {
  
  // look for tone
  if (radio.getDTMFSample() != 0) {
    uint16_t code = radio.getDTMFCode();
    if (code < 10) {
      Serial.println(code);
    } else if (code < 0xE) {
      Serial.println(code, HEX);
    } else if (code == 0xE) {
      Serial.println('*');
    } else if (code == 0xF) {
      Serial.println('#');
    } else {
      Serial.println('?'); // invalid code
    }
    while (radio.getDTMFSample() == 1) {
      // wait until this code is done
      delay(10);
    }
  }
  
  // Is it time to send tone?
  if (Serial.available()) {
    char c = Serial.read();
    uint8_t code;
    if (c == '#') {
      code = 0xF;
    } else if (c=='*') {
      code == 0xE;
    } else if (c >= 'A' && c <= 'D') {
      code = c - 'A' + 0xA;
    } else if (c >= '0' && c <= '9') {
      code = c - '0';
    } else {
      // invalid code, skip it
      Serial.println('?');
      return;
    }
    
    // set tones
    radio.setDTMFCode(code);
    // start transmitting
    radio.setTxSourceTones();
    radio.setModeTransmit();
    // TODO: may need to set DTMF enable again
    
    // wait until done
    while (radio.getDTMFTxActive() != 1) {
      // wait until we're ready for a new code
      delay(10);
    }
    radio.setDTMFCode(code);
    // TODO: fix timing
    //while (radio.getDTMFTxActive() != 0) {
      // wait until this code is done
      delay(1000);
    //}

    // done with tone
    radio.setModeReceive();
    radio.setTxSourceMic();
  }
}
