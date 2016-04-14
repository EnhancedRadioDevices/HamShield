// Hamshield 

#include <HamShield.h>

// create object for radio
HamShield radio;

#define LED_PIN 13
#define RSSI_REPORT_RATE_MS 5000

//TODO: move these into library
#define PWM_PIN 3
#define RESET_PIN A3
#define SWITCH_PIN 2

bool blinkState = false;
bool currently_tx;

uint32_t freq;

unsigned long rssi_timeout;

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
  
/*
  // set to transmit
  radio.setModeTransmit();
  // maybe set PA bias voltage
  Serial.println("configured for transmit");
  radio.setTxSourceMic();
  
  
  */  
  radio.setRfPower(0);
    
  // configure Arduino LED for
  pinMode(LED_PIN, OUTPUT);
  rssi_timeout = 0;

}

void loop() {  
  if (!digitalRead(SWITCH_PIN))
  {
    if (!currently_tx) 
    {
      currently_tx = true;
      
      // set to transmit
      radio.setModeTransmit();
      Serial.println("Tx");
      //radio.setTxSourceMic();
      //radio.setRfPower(1);
    }
  } else if (currently_tx) {
    radio.setModeReceive();
    currently_tx = false;
    Serial.println("Rx");
  }
  
  
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
  
  if (!currently_tx && (millis() - rssi_timeout) > RSSI_REPORT_RATE_MS)
  {
    Serial.println(radio.readRSSI());
    rssi_timeout = millis();
  }
}
