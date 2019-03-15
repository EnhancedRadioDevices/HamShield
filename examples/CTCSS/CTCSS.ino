/* Hamshield
 * Example: CTCSS
 * This is a simple example to demonstrate HamShield receive
 * and transmit functionality using CTCSS. The HamShield will
 * have audio output muted until it receives the correct
 * sub-audible tone. It does this by polling a tone detection
 * flag on the HamShield, but it's also possible to do this
 * using interrupts if you connect GPIO0 from the HamShield
 * to your Arduino (code for that not provided).
 * 
 * Setup:
 * Connect the HamShield to your Arduino. Screw the antenna 
 * into the HamShield RF jack. Plug a pair of headphones into 
 * the HamShield. Connect the Arduino to wall power and then 
 * to your computer via USB. Set the CTCSS tone that you
 * want to use in the setup() function below.
 * After uploading this program to your Arduino, open the 
 * Serial Monitor. Press the button on the HamShield to begin 
 * setup. The sketch then works exactly like the HandyTalkie
 * example, with the exception that only valid CTCSS coded
 * receptions are put out to the headset.
*/

#include <HamShield.h>

// create object for radio
HamShield radio;

#define LED_PIN 13
#define RSSI_REPORT_RATE_MS 5000

#define MIC_PIN 3
#define RESET_PIN A3
#define SWITCH_PIN 2

bool currently_tx;

uint32_t freq;
float ctcss_tone;
bool muted;

unsigned long rssi_timeout;

void setup() {
  // NOTE: if not using PWM out, it should be held low to avoid tx noise
  pinMode(MIC_PIN, OUTPUT);
  digitalWrite(MIC_PIN, LOW);
  
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
  delay(5); // wait for device to come up
  
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
  Serial.println("changing frequency");
  
  radio.setSQOff();
  freq = 446000;
  radio.frequency(freq);
  
  // set to receive
  
  radio.setModeReceive();
  currently_tx = false;
  Serial.print("config register is: ");
  Serial.println(radio.readCtlReg());
  Serial.println(radio.readRSSI());
   
  radio.setRfPower(0);

  // CTCSS Setup code
  ctcss_tone = 131.8;
  radio.setCtcss(ctcss_tone);
  radio.enableCtcss();
  Serial.print("ctcss tone: ");
  Serial.println(radio.getCtcssFreqHz());
  // mute audio until we get a CTCSS tone
  radio.setMute();
  muted = true;
    
  // configure Arduino LED for
  pinMode(LED_PIN, OUTPUT);
  rssi_timeout = 0;

}

void loop() {  
  // handle CTCSS tone detection
  if (!currently_tx) {
    // check for CTCSS tone
    if (radio.getCtcssToneDetected()) {
      if (muted) {
        muted = false;
        radio.setUnmute();
        Serial.println("tone");
      }
    } else {
      if (!muted) {
        muted = true;
        radio.setMute();
        Serial.println("no tone");
      }
    }
  }

  // handle manual transmit
  if (!digitalRead(SWITCH_PIN))
  {
    if (!currently_tx) 
    {
      currently_tx = true;
      
      // set to transmit
      radio.setModeTransmit();
      Serial.println("Tx");

      radio.setUnmute(); // can't mute during transmit
      muted = false;
    }
  } else if (currently_tx) {
    radio.setModeReceive();
    currently_tx = false;
    Serial.println("Rx");

    radio.setMute(); // default to mute during rx
    muted = true;
  }
  
  // handle serial commands
  if (Serial.available()) {
    if (Serial.peek() == 'r') {
      Serial.read();
      digitalWrite(RESET_PIN, LOW);
      delay(1000);
      digitalWrite(RESET_PIN, HIGH);
      radio.initialize(); // initializes automatically for UHF 12.5kHz channel
    } else {
      Serial.setTimeout(40);
      freq = Serial.parseInt();
      Serial.flush();
      radio.frequency(freq);
      Serial.print("set frequency: ");
      Serial.println(freq);
    }
  }

  // periodically read RSSI and print to screen
  if (!currently_tx && (millis() - rssi_timeout) > RSSI_REPORT_RATE_MS)
  {
    Serial.println(radio.readRSSI());
    rssi_timeout = millis();
  }
}