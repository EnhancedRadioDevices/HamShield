/* Hamshield
 * Example: Serial Tranceiver
 * SerialTransceiver is TTL Serial port "glue" to allow 
 * desktop or laptop control of the HamShield.
 * Connect the HamShield to your Arduino. Screw the antenna 
 * into the HamShield RF jack. Plug a pair of headphones into 
 * the HamShield. Connect the Arduino to wall power and then 
 * to your computer via USB. After uploading this program to 
 * your Arduino, open the Serial Monitor. Use the bar at the 
 * top of the serial monitor to enter commands as seen below.
 * 
 * EXAMPLE: To change the repeater offset to 144.425MHz, 
 * enable offset, then key in, use the following commands:
 * T144425;
 * R1;
 * [Just a space]

 
// see also: https://github.com/EnhancedRadioDevices/HamShield/wiki/HamShield-Serial-Mode
 
Commands:

Mode           ASCII       Description                                                                                                                                  
-------------- ----------- -------------------------------------------------------------------------------------------------------------------------------------------- 
Transmit       space       Space must be received at least every 500 mS                                                                                                 
Receive        not space   If space is not received and/or 500 mS timeout of space occurs, unit will go into receive mode                                               
Frequency      F<freq>;    Set the receive frequency in KHz, if offset is disabled, this is the transmit frequency                                                      
Morse Out      M<text>;    A small buffer for morse code (32 chars)
Morse In       N;          Sets mode to Morse In, listening for Morse
Power level    P<level>;   Set the power amp level, 0 = lowest, 15 = highest
Enable Offset  R<state>;   1 turns on repeater offset mode, 0 turns off repeater offset mode
Squelch        S<level>;   Set the squelch level
TX Offset      T<freq>;    The absolute frequency of the repeater offset to transmit on in KHz
RSSI           ?           Respond with the current receive level in - dBm (no sign provided on numerical response)
Voice Level    ^           Respond with the current voice level (VSSI), only valid when transmitting
DTMF Out       D<vals>;    A small buffer for DTMF out (only 0-9,A,B,C,D,*,# accepted)
DTMF In        B;          Sets mode to DTMF In, listening for DTMF
PL Tone Tx     A<val>;     Sets PL tone for TX, value is tone frequency in Hz (float), set to 0 to disable
PL Tone Rx     C<val>;     Sets PL tone for RX, value is tone frequency in Hz (float), set to 0 to disable
Volume 1       V1<val>;    Set volume 1 (value between 0 and 15)
Volume 2       V2<val>;    Set volume 2 (value between 0 and 15)
KISS TNC       K;          Move to KISS TNC mode (send ^; to move back to normal mode). NOT IMPELEMENTED YET
Normal Mode    _           Move to Normal mode from any other mode (except TX)

Responses:

Condition    ASCII      Description
------------ ---------- -----------------------------------------------------------------
Startup      *<code>;   Startup and shield connection status
Success      !;         Generic success message for command that returns no value
Error        X<code>;   Indicates an error code. The numerical value is the type of error
Value        :<value>;  In response to a query
Status       #<value>;  Unsolicited status message
Debug Msg    @<text>;   32 character debug message
Rx Msg       R<text>;   up to 32 characters of received message, only if device is in DTMF or Morse Rx modes

*/


// Note that the following are not yet implemented
// TODO: change get_value so it's intuitive
// TODO: Squelch open and squelch shut independently controllable
// TODO: pre/de emph filter
// TODO: walkie-talkie
// TODO: KISS TNC

#include "HamShield.h"

#define MIC_PIN 3
#define RESET_PIN A3
#define SWITCH_PIN 2

enum {TX, NORMAL, DTMF, MORSE, KISS};

int state = NORMAL;
bool rx_ctcss = false;
bool muted = false;

int txcount = 0;
long timer = 0; // Transmit timer to track timeout (send space to reset)

long freq = 432100; // 70cm calling frequency, receive frequency and default transmit frequency
long tx_freq = 0; // transmit frequency if repeater is on
int pwr = 0; // tx power

char cmdbuff[32] = "";
int temp = 0;

bool repeater = false; // true if transmit and receive operate on different frequencies
char pl_rx_buffer[32]; // pl tone rx buffer
char pl_tx_buffer[32]; // pl tone tx buffer

float ctcssin = 0;
float ctcssout = 0;
int cdcssin = 0;
int cdcssout = 0;


HamShield radio;

void setup() {
  // NOTE: if not using PWM out (MIC pin), it should be held low to avoid tx noise
  pinMode(MIC_PIN, OUTPUT);
  digitalWrite(MIC_PIN, LOW);
  
  // prep the switch
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  
  // set up the reset control pin
  // NOTE: HamShieldMini doesn't have a reset pin, so this has no effect
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, HIGH);
  
  Serial.begin(9600);
  Serial.println(";;;;;;;;;;;;;;;;;;;;;;;;;;");

  int result = radio.testConnection();
  Serial.print("*");
  Serial.print(result,DEC);
  Serial.println(";");
  radio.initialize(); // initializes automatically for UHF 12.5kHz channel
  radio.frequency(freq);
  radio.setVolume1(0xF);
  radio.setVolume2(0xF);
  radio.setModeReceive();
  radio.setTxSourceMic();
  radio.setRfPower(pwr);
  radio.setSQLoThresh(-80);
  radio.setSQHiThresh(-70);
  radio.setSQOn();
  Serial.println("*START;");
}

void loop() {
  
  if(Serial.available()) { 

    int text = Serial.read(); // get the first char to see what the upcoming command is

    switch (state) {
      // we handle commands differently based on what state we're in
      
      case TX:
         // we're currently transmitting
         // if we got a space, reset our transmit timeout
         if(text == ' ') { timer = millis();}
         break;

      case NORMAL:
        switch(text) {
          case ' ':  // space - transmit
            if(repeater == true && tx_freq != 0) { radio.frequency(tx_freq); }
            muted = false; // can't mute (for PL tones) during tx
            radio.setUnmute(); 
            radio.setModeTransmit();
            state = TX;
            Serial.println("#TX,ON;");
            timer = millis();
            break;
           
          case '?': // ? - RSSI
            Serial.print(":");
            Serial.print(radio.readRSSI(),DEC);
            Serial.println(";");
            break;
               
          case '^': // ^ - VSSI (voice) level
            Serial.print(":");
            Serial.print(radio.readVSSI(),DEC); 
            Serial.println(";");
            break;
               
          case 'F': // F - frequency
            getValue();
            freq = atol(cmdbuff);
            if(radio.frequency(freq) == true) { 
              Serial.print("@"); 
              Serial.print(freq,DEC); 
              Serial.println(";!;"); 
            } else { 
              Serial.println("X1;"); 
            } 
            break;
               
          case 'P': // P - power level
            getValue();
            temp = atol(cmdbuff);
            radio.setRfPower(temp);
            Serial.println("!;"); 
            break;
           
          case 'S': // S - squelch
            getValue();
            temp = atol(cmdbuff);
            if (temp < -2 && temp > -130) {
              radio.setSQLoThresh(temp);
              radio.setSQHiThresh(temp+2);
              radio.setSQOn();
              Serial.print(temp);
              Serial.println("!;"); 
            } else {
              Serial.println("X!;");
            }
            break;
           
          case 'R': // R - repeater offset mode
            getValue();
            temp = atol(cmdbuff);
            if(temp == 0) { repeater = 0; }
            if(temp == 1) { repeater = 1; }
            Serial.println("!;"); 
            break;
           
          case 'T': // T - transmit offset 
            getValue();
            tx_freq = atol(cmdbuff);
            Serial.println("!;"); 
            break;
           
          case 'M': // M - Morse
            getValue();
            if(repeater == true && tx_freq != 0) { radio.frequency(tx_freq); }
            muted = false; // can't mute (for PL tones) during tx
            radio.setUnmute(); 
            radio.setModeTransmit();
            delay(300);
            radio.morseOut(cmdbuff);
            if(repeater == true) { radio.frequency(freq); } 
            radio.setModeReceive();
            Serial.println("!;"); 
            break; 
           
          case 'N': // N - set to Morse in Mode
            morse_rx_setup();
            state = MORSE;
            Serial.println("!;"); 
            break;
               
          case 'D': // D - DTMF Out
            dtmfSetup();
            getValue();
            dtmf_out(cmdbuff);
            Serial.println("!;"); 
            break;

          case 'B': // B - set to DTMF in Mode
            dtmfSetup();
            radio.enableDTMFReceive();
            state = DTMF;
            Serial.println("!;"); 
            break;

          case 'A': // A - TX PL Tone configuration command
            pl_tone_tx();
            Serial.println("!;"); 
            break;
          
          case 'C': // C - RX PL Tone configuration command
            pl_tone_rx();
            Serial.println("!;"); 
            break;

          case 'V': // V - set volume
            getValue();
            temp = cmdbuff[0];
            if (temp == 0x31) {
              temp = atol(cmdbuff + 1);
              radio.setVolume1(temp);
              Serial.println("!;"); 
            } else if (temp == 0x32) {
              temp = atol(cmdbuff + 1);
              radio.setVolume2(temp);
              Serial.println("!;"); 
            } else {
              // not a valid volume command
              while (Serial.available()) { Serial.read(); }
              Serial.println("X!;");
            }
            break;

          case 'K': // K - switch to KISS TNC mode
            //state = KISS;
            //TODO: set up KISS
            Serial.println("X1;"); 
            break;

          default:
            // unknown command, flush the input buffer and wait for next one
            Serial.println("X1;");
            while (Serial.available()) { Serial.read(); }
            break;
         }
         break;

       case KISS:
         if (Serial.peek() == '_') {
           state = NORMAL;
           if (rx_ctcss) {
             radio.enableCtcss();
             muted = true; // can't mute (for PL tones) during tx
             radio.setMute();
           }
         }
         // TODO: handle KISS TNC
         break;

       case MORSE:
         if (text == '_') { state = NORMAL; }
         if (text == 'M') { // tx message
            getValue();
            if(repeater == true && tx_freq != 0) { radio.frequency(tx_freq); } 
            muted = false; // can't mute (for PL tones) during tx
            radio.setUnmute();
            radio.setModeTransmit();
            delay(300);
            radio.morseOut(cmdbuff);
            if(repeater == true) { radio.frequency(freq); } 
            radio.setModeReceive();
         } else {
           // not a valid cmd
           while (Serial.available()) { Serial.read(); }
         }
         break;

       case DTMF:
         if (text == '_') { state = NORMAL; }
         if (text == 'D') { // tx message
            getValue();
            dtmf_out(cmdbuff);
         } else {
           // not a valid cmd
           while (Serial.available()) { Serial.read(); }
         }
         break;

       default:
         // we're in an invalid state, reset to safe settings
         while (Serial.available()) { Serial.read(); }
         radio.frequency(freq);
         radio.setModeReceive();
         state = NORMAL;
         break;
     }

  }
  
  // now handle any state related functions
  switch (state) {
    case TX:
      if(millis() > (timer + 500)) { 
        Serial.println("#TX,OFF;");
        radio.setModeReceive(); 
        if(repeater == true) { radio.frequency(freq); }
        if (rx_ctcss) {
          radio.setMute();
          muted = true;
        }
        txcount = 0;
        state = NORMAL;
      }
      break;

    case NORMAL:
      // deal with rx ctccs if necessary
      if (rx_ctcss) {
        if (radio.getCtcssToneDetected()) {
          if (muted) {
            muted = false;
            radio.setUnmute();
          }
        } else {
          if (!muted) {
            muted = true;
            radio.setMute();
          }
        }
      }
      break;

    case DTMF:
      dtmf_rx(); // wait for DTMF reception
      break;

    case MORSE:
      morse_rx(); // wait for Morse reception
      break;
  }

  // get rid of any trailing whitespace in the serial buffer
  if (Serial.available()) {
    char cpeek = Serial.peek();
    while (cpeek == ' ' || cpeek == '\r' || cpeek == '\n')
    {
      Serial.read();
      cpeek = Serial.peek();
    }
    
  }
}

void getValue() {
  int p = 0;
  char temp;
  for(;;) {
    if(Serial.available()) { 
      temp = Serial.read();
      if(temp == 59) { 
        cmdbuff[p] = 0; 
        return;
      }
      cmdbuff[p] = temp;
      p++;
      if(p == 32) { 
        cmdbuff[0] = 0; 
        return;
      }
    }
  }
}

void dtmfSetup() {
  radio.setVolume1(6);
  radio.setVolume2(0);
  radio.setDTMFDetectTime(24); // time to detect a DTMF code, units are 2.5ms
  radio.setDTMFIdleTime(50); // time between transmitted DTMF codes, units are 2.5ms
  radio.setDTMFTxTime(60); // duration of transmitted DTMF codes, units are 2.5ms
}

void dtmf_out(char * out_buf) {
  if (out_buf[0] == ';' || out_buf[0] == 0) return; // empty message
    
  uint8_t i = 0;
  uint8_t code = radio.DTMFchar2code(out_buf[i]);
    
  // start transmitting
  radio.setDTMFCode(code); // set first
  radio.setTxSourceTones();
  if(repeater == true && tx_freq != 0) { radio.frequency(tx_freq); }
  muted = false; // can't mute during transmit
  radio.setUnmute();
  radio.setModeTransmit();
  delay(300); // wait for TX to come to full power

  bool dtmf_to_tx = true;
  while (dtmf_to_tx) {
    // wait until ready
    while (radio.getDTMFTxActive() != 1) {
      // wait until we're ready for a new code
      delay(10);
    }
    if (i < 32 && out_buf[i] != ';' && out_buf[i] != 0) {
      code = radio.DTMFchar2code(out_buf[i]);
      if (code == 255) code = 0xE; // throw a * in there so we don't break things with an invalid code
      radio.setDTMFCode(code); // set first
    } else {
      dtmf_to_tx = false;
      break;
    }
    i++;

    while (radio.getDTMFTxActive() != 0) {
      // wait until this code is done
      delay(10);
    }
  }
  // done with tone
  radio.setModeReceive();
  if (repeater == true) {radio.frequency(freq);}
  radio.setTxSourceMic();
}

void dtmf_rx() {
  char m = radio.DTMFRxLoop();
  if (m != 0) {
    // Note: not doing buffering of messages, 
    // we just send a single morse character
    // whenever we get it
    Serial.print('R');
    Serial.print(m);
    Serial.println(';');
  }
}

// TODO: morse config info

void morse_rx_setup() {
  // Set the morse code characteristics
  radio.setMorseFreq(MORSE_FREQ);
  radio.setMorseDotMillis(MORSE_DOT);
  
  radio.lookForTone(MORSE_FREQ);
  
  radio.setupMorseRx();
}

void morse_rx() {
  char m = radio.morseRxLoop();

  if (m != 0) {
     // Note: not doing buffering of messages, 
     // we just send a single morse character
     // whenever we get it
     Serial.print('R');
     Serial.print(m);
     Serial.println(';');
  }
}

void pl_tone_tx() { 
  memset(pl_tx_buffer,0,32);
  uint8_t ptr = 0;
  while(1) { 
    if(Serial.available()) { 
      uint8_t buf = Serial.read();
      if(buf == 'X') { return; }
      if(buf == ';') { pl_tx_buffer[ptr] = 0; program_pl_tx(); return; }
      if(ptr == 31) { return; }
      pl_tx_buffer[ptr] = buf; ptr++; 
    }
  }
}

void program_pl_tx() { 
  float pl_tx = atof(pl_tx_buffer);
  radio.setCtcss(pl_tx);

  if (pl_tx == 0) {
    radio.disableCtcssTx();
  } else {
    radio.enableCtcssTx();
  }
}

void pl_tone_rx() { 
  memset(pl_rx_buffer,0,32);
  uint8_t ptr = 0;
  while(1) { 
    if(Serial.available()) { 
    uint8_t buf = Serial.read();
    if(buf == 'X') { return; }
    if(buf == ';') { pl_rx_buffer[ptr] = 0; program_pl_rx(); return; }
    if(ptr == 31) { return; }
    pl_rx_buffer[ptr] = buf; ptr++; 
    }
  }
}

void program_pl_rx() {
  float pl_rx = atof(pl_rx_buffer);
  radio.setCtcss(pl_rx);
  if (pl_rx == 0) {
    rx_ctcss = false;
    radio.setUnmute();
    muted = false;
    radio.disableCtcssRx();
  } else {
    rx_ctcss = true;
    radio.setMute();
    muted = true;
    radio.enableCtcssRx();
  }
}
