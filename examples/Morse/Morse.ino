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
*/

#define DDS_REFCLK_DEFAULT 9600
#include <HamShield.h>

#define MIC_PIN 3
#define RESET_PIN A3
#define SWITCH_PIN 2

#define MORSE_FREQ 600
#define MORSE_DOT 100 // ms
// Note that all timing is defined in terms of MORSE_DOT relative durations
// You may want to tweak those timings below

#define SYMBOL_END_TIME 5 //millis
#define CHAR_END_TIME (MORSE_DOT*2.3)
#define MESSAGE_END_TIME (MORSE_DOT*15)

#define MIN_DOT_TIME (MORSE_DOT*0.7)
#define MAX_DOT_TIME (MORSE_DOT*1.3)
#define MIN_DASH_TIME (MORSE_DOT*2.7)
#define MAX_DASH_TIME (MORSE_DOT*3.3)


HamShield radio;

uint32_t last_tone_check; // track how often we check for morse tones
uint32_t tone_in_progress; // track how long the current tone lasts
uint32_t space_in_progress; // track how long since the last tone
uint8_t rx_morse_char;
uint8_t rx_morse_bit;

char rx_msg[128];
uint8_t rx_idx;

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

  // Configure the HamShield to operate on 438.000MHz
  radio.frequency((uint32_t) 438000);
  radio.setModeReceive();
  
  Serial.println("Radio Configured.");
  last_tone_check = millis();
  space_in_progress = 0; // haven't checked yet
  tone_in_progress = 0; // not currently listening to a tone
  rx_morse_char = 0; // haven't found any tones yet
  rx_idx = 0;
  rx_morse_bit = 1;
}

void loop() {
  // are we receiving anything
  if (radio.toneDetected()) {
    space_in_progress = 0;
    if (tone_in_progress == 0) {
      // start a new tone
      tone_in_progress = millis();
    }
  } else {
    // keep track of how long the silence is
    if (space_in_progress == 0) space_in_progress = millis();

    // we wait for a bit of silence before ending the last
    // symbol in order to smooth out the detector
    if ((millis() - space_in_progress) > SYMBOL_END_TIME)
    {
      if (tone_in_progress != 0) {
        // end the last tone
        uint16_t tone_time = millis() - tone_in_progress;
        tone_in_progress = 0;
        handleTone(tone_time);
      } 
    } 

    // we might be done with a character if the space is long enough
    if ((millis() - space_in_progress) > CHAR_END_TIME) {
      char m = parseMorse();
      if (m != 0) {
        rx_msg[rx_idx++] = m;
      }
    }

    // we might be done with a message if the space is long enough
    if ((millis() - space_in_progress) > MESSAGE_END_TIME) {
      if (rx_idx > 0) {
        // we got a message, print it now
        rx_msg[rx_idx] = '\0'; // null terminate
        Serial.println(rx_msg);
        rx_idx = 0; // reset message buffer
      }
      rx_morse_char = 0;
      rx_morse_bit = 1;
    }
  }
     
  // should we send anything
  if (Serial.available()) {
    Serial.println("checking channel");
    // We'll wait up to 30 seconds for a clear channel, requiring that the channel is clear for 2 seconds before we transmit
    if (radio.waitForChannel(30000,2000,-5)) {
      // If we get here, the channel is clear. 

      // Start transmitting by putting the radio into transmit mode.
      radio.setModeTransmit();
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
      radio.setModeReceive();
      Serial.println("sent");
    } else {
      // If we get here, the channel is busy. Let's also print out the RSSI.
      Serial.print("The channel was busy. RSSI: ");
      Serial.println(radio.readRSSI());
    }
  }
}

void handleTone(uint16_t tone_time) {
  //Serial.println(tone_time);
  if (tone_time > MIN_DOT_TIME && tone_time < MAX_DOT_TIME) {
    // add a dot
    //Serial.print(".");
    //nothing to do for this bit position, since . = 0
  } else if (tone_time > MIN_DASH_TIME && tone_time < MAX_DASH_TIME) {
    // add a dash
    //Serial.print("-");
    rx_morse_char += rx_morse_bit;
  }

  // prep for the next bit
  rx_morse_bit = rx_morse_bit << 1;
}

char parseMorse() {
  // if morse_char is a valid morse character, return the character
  // if morse_char is an invalid (incomplete) morse character, return 0


  //if (rx_morse_bit != 1) Serial.println(rx_morse_char, BIN);
  rx_morse_char += rx_morse_bit; // add the terminator bit
  // if we got a char, then print it
  char c = radio.morseReverseLookup(rx_morse_char);
  rx_morse_char = 0;
  rx_morse_bit = 1;
  return c;
}

