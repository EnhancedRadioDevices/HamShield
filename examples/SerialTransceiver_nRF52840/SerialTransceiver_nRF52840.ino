/* Hamshield
 * Example: AppSerialController_nRF52840
 * This is a simple example to demonstrate the HamShield working 
 * with an Adafruit Feather nRF52840 Express
 * 
 * HamShield to Feather Connections:
 *  SPKR - Feather A0
 *  MIC - Feather D11
 *  CLK - Feather D5
 *  nCS - Feather D6
 *  DAT - Feather D9
 *  GND - Feather GND
 *  VCC - Feather 3.3V
 *  
 * Connect the HamShield to your Feather as above. 
 * Screw the antenna into the HamShield RF jack. Plug a pair 
 * of headphones into the HamShield.
 * 
 * Connect the Feather nRF52840 Express to your computer via 
 * a USB Micro B cable. After uploading this program to 
 * your Feather, open the Serial Monitor. You should see some
 * text displayed that documents the setup process.
 * 
 * Once the Feather is set up and talking to the HamShield,
 * you can control it over USB-Serial or BLE-Serial(UART).
 * 
 * Try using Adafruit's Bluefruit app to connect to the Feather.
 * Once you're connected, you can control the HamShield using
 * the same commands you'd use over USB-Serial. The response to
 * all commands will be echoed to both USB-Serial and BLE-Serial(UART).
 * 

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

#include <bluefruit.h>
#include <stdarg.h>
#include <stdio.h>
#include <HamShield.h>

// BLE Service
BLEDis  bledis;  // device information
BLEUart bleuart; // uart over ble
BLEBas  blebas;  // battery


// create object for radio
HamShield radio(6,5,9);
// To use non-standard pins, use the following initialization
//HamShield radio(ncs_pin, clk_pin, dat_pin);

#define LED_PIN 3

#define MIC_PIN A1


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

void setup() {
  // NOTE: if not using PWM out (MIC pin), it should be held low to avoid tx noise
  pinMode(MIC_PIN, OUTPUT);
  digitalWrite(MIC_PIN, LOW);
  
  // initialize serial communication
  Serial.begin(115200);
  while (!Serial) delay(10);

  // Setup the BLE LED to be enabled on CONNECT
  // Note: This is actually the default behaviour, but provided
  // here in case you want to control this LED manually via PIN 19
  Bluefruit.autoConnLed(true);

  // Config the peripheral connection with maximum bandwidth 
  // more SRAM required by SoftDevice
  // Note: All config***() function must be called before begin()
  Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);

  Bluefruit.begin();
  // Set max power. Accepted values are: -40, -30, -20, -16, -12, -8, -4, 0, 4
  Bluefruit.setTxPower(4);
  Bluefruit.setName("MyBlueHam");
  //Bluefruit.setName(getMcuUniqueID()); // useful testing with multiple central connections
  Bluefruit.setConnectCallback(connect_callback);
  Bluefruit.setDisconnectCallback(disconnect_callback);

  // Configure and Start Device Information Service
  bledis.setManufacturer("Enhanced Radio Devices");
  bledis.setModel("BlueHam");
  bledis.begin();

  // Configure and Start BLE Uart Service
  bleuart.begin();

  // Start BLE Battery Service
  blebas.begin();
  blebas.write(100);

  // Set up and start advertising
  startAdv();
    
  delay(100);  

  SerialWrite(";;;;;;;;;;;;;;;;;;;;;;;;;;\n");

  int result = radio.testConnection();
  SerialWrite("*%d;\n", result);
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
  SerialWrite("*START;\n");
}

void startAdv(void)
{
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();

  // Include bleuart 128-bit uuid
  Bluefruit.Advertising.addService(bleuart);

  // Secondary Scan Response packet (optional)
  // Since there is no room for 'Name' in Advertising packet
  Bluefruit.ScanResponse.addName();
  
  /* Start Advertising
   * - Enable auto advertising if disconnected
   * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
   * - Timeout for fast mode is 30 seconds
   * - Start(timeout) with timeout = 0 will advertise forever (until connected)
   * 
   * For recommended advertising interval
   * https://developer.apple.com/library/content/qa/qa1931/_index.html   
   */
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds  
}

void loop() {  
  // TODO: loop fixing based on serialtransciever!

  char c = 0;
  bool ble_serial = false;
  if (Serial.available()) {
    Serial.readBytes(&c, 1);
  } else if (bleuart.available()) {
    c = (char) bleuart.read();
    ble_serial = true;
  }

  // TODO: BLE
  if(c != 0) { 

    int text = c; // get the first char to see what the upcoming command is

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
            SerialWrite("#TX,ON;\n");
            timer = millis();
            break;
           
          case '?': // ? - RSSI
            SerialWrite(":%d;\n", radio.readRSSI());
            break;
               
          case '^': // ^ - VSSI (voice) level
            SerialWrite(":%d;\n", radio.readVSSI());
            break;
               
          case 'F': // F - frequency
            getValue(ble_serial);
            freq = atol(cmdbuff);
            if(radio.frequency(freq) == true) { 
              SerialWrite("@%d;!;\n", freq); 
            } else { 
              SerialWrite("X1;\n"); 
            } 
            break;
               
          case 'P': // P - power level
            getValue(ble_serial);
            temp = atol(cmdbuff);
            radio.setRfPower(temp);
            SerialWrite("!;\n"); 
            break;
           
          case 'S': // S - squelch
            getValue(ble_serial);
            temp = atol(cmdbuff);
            if (temp < -2 && temp > -130) {
              radio.setSQLoThresh(temp);
              radio.setSQHiThresh(temp+2);
              radio.setSQOn();
              SerialWrite("%d!;\n", temp);
            } else {
              SerialWrite("X!;\n");
            }
            break;
           
          case 'R': // R - repeater offset mode
            getValue(ble_serial);
            temp = atol(cmdbuff);
            if(temp == 0) { repeater = 0; }
            if(temp == 1) { repeater = 1; }
            SerialWrite("!;\n"); 
            break;
           
          case 'T': // T - transmit offset 
            getValue(ble_serial);
            tx_freq = atol(cmdbuff);
            SerialWrite("!;\n"); 
            break;
           
          case 'M': // M - Morse
            getValue(ble_serial);
            if(repeater == true && tx_freq != 0) { radio.frequency(tx_freq); }
            muted = false; // can't mute (for PL tones) during tx
            radio.setUnmute(); 
            radio.setModeTransmit();
            delay(300);
            radio.morseOut(cmdbuff);
            if(repeater == true) { radio.frequency(freq); } 
            radio.setModeReceive();
            SerialWrite("!;\n"); 
            break; 
           
          case 'N': // N - set to Morse in Mode
            morse_rx_setup();
            state = MORSE;
            SerialWrite("!;\n"); 
            break;
               
          case 'D': // D - DTMF Out
            dtmfSetup();
            getValue(ble_serial);
            dtmf_out(cmdbuff);
            SerialWrite("!;\n"); 
            break;

          case 'B': // B - set to DTMF in Mode
            dtmfSetup();
            radio.enableDTMFReceive();
            state = DTMF;
            SerialWrite("!;\n"); 
            break;

          case 'A': // A - TX PL Tone configuration command
            pl_tone_tx();
            SerialWrite("!;\n"); 
            break;
          
          case 'C': // C - RX PL Tone configuration command
            pl_tone_rx();
            SerialWrite("!;\n"); 
            break;

          case 'V': // V - set volume
            getValue(ble_serial);
            temp = cmdbuff[0];
            if (temp == 0x31) {
              temp = atol(cmdbuff + 1);
              radio.setVolume1(temp);
              SerialWrite("!;\n"); 
            } else if (temp == 0x32) {
              temp = atol(cmdbuff + 1);
              radio.setVolume2(temp);
              SerialWrite("!;\n"); 
            } else {
              // not a valid volume command, flush buffers
              SerialFlush(ble_serial);
              SerialWrite("X!;\n");
            }
            break;

          case 'K': // K - switch to KISS TNC mode
            //state = KISS;
            //TODO: set up KISS
            SerialWrite("X1;\n"); 
            break;

          default:
            // unknown command, flush the input buffer and wait for next one
            SerialWrite("X1;\n");
            SerialFlush(ble_serial);
            break;
         }
         break;

       case KISS:
         if ((ble_serial && bleuart.peek() == '_') || (!ble_serial && Serial.peek() == '_')) {
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
            getValue(ble_serial);
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
           SerialFlush(ble_serial);
         }
         break;

       case DTMF:
         if (text == '_') { state = NORMAL; }
         if (text == 'D') { // tx message
            getValue(ble_serial);
            dtmf_out(cmdbuff);
         } else {
           // not a valid cmd
           SerialFlush(ble_serial);
         }
         break;

       default:
         // we're in an invalid state, reset to safe settings
         SerialFlush(ble_serial);
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
        SerialWrite("#TX,OFF;\n");
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
  SerialFlushWhitespace(ble_serial);
}


// callback invoked when central connects
void connect_callback(uint16_t conn_handle)
{
  char central_name[32] = { 0 };
  Bluefruit.Gap.getPeerName(conn_handle, central_name, sizeof(central_name));

  Serial.print("Connected to ");
  Serial.println(central_name);
}

/**
 * Callback invoked when a connection is dropped
 * @param conn_handle connection where this event happens
 * @param reason is a BLE_HCI_STATUS_CODE which can be found in ble_hci.h
 * https://github.com/adafruit/Adafruit_nRF52_Arduino/blob/master/cores/nRF5/nordic/softdevice/s140_nrf52_6.1.1_API/include/ble_hci.h
 */
void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
  (void) conn_handle;
  (void) reason;

  Serial.println();
  Serial.println("Disconnected");
}


void getValue(bool ble_serial) {  
  int p = 0;
  char temp;
  
  for(;;) {
    if((!ble_serial && Serial.available()) || (ble_serial && bleuart.available())) { 
      if (ble_serial) {
        temp = bleuart.read();
      } else {
        temp = Serial.read();
      }
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
    SerialWrite("R%d;\n", m);
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
     SerialWrite("R%c;\n",m);
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

#define TEXT_BUF_LEN 64
char text_buf[TEXT_BUF_LEN];
void SerialWrite(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);    
    int str_len = vsnprintf(text_buf, TEXT_BUF_LEN, fmt, args);
    va_end(args);

    bleuart.write(text_buf, str_len);
    Serial.write(text_buf, str_len);
}

void SerialFlush(bool ble_serial) {
  if (ble_serial) {
    while (bleuart.available()) { bleuart.read(); }
  } else {
    while (Serial.available()) { Serial.read(); }
  }
}


void SerialFlushWhitespace(bool ble_serial) {
  if (!ble_serial && Serial.available()) {
    char cpeek = Serial.peek();
    while (cpeek == ' ' || cpeek == '\r' || cpeek == '\n')
    {
      Serial.read();
      cpeek = Serial.peek();
    }    
  } else if (ble_serial && bleuart.available()) {
    char cpeek = bleuart.peek();
    while (cpeek == ' ' || cpeek == '\r' || cpeek == '\n')
    {
      bleuart.read();
      cpeek = bleuart.peek();
    }    
  }
}