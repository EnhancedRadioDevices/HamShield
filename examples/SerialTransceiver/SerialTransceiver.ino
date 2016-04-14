/*

SerialTransceiver is TTL Serial port "glue" to allow desktop or laptop control of the HamShield

Commands:

Mode           ASCII       Description                                                                                                                                  Implemented
-------------- ----------- -------------------------------------------------------------------------------------------------------------------------------------------- -----------------
Transmit       space       Space must be received at least every 500 mS                                                                                                 Yes
Receive        not space   If space is not received and/or 500 mS timeout of space occurs, unit will go into receive mode                                               Yes
CTCSS In       A<tone>;    <tone> must be a numerical ascii value with decimal point indicating CTCSS receive tone required to unsquelch                                No
CTCSS Out      B<tone>;    <tone> must be a numerical ascii value with decimal point indicating CTCSS transmit tone                                                     No
CTCSS Enable   C<state>;   Turns on CTCSS mode (analog tone) with 1, off with 0.                                                                                        No
CDCSS Enable   D<state>;   Turns on CDCSS mode (digital tone) with 1, off with 0.                                                                                       No
Bandwidth      E<mode>;    for 12.5KHz mode is 0, for 25KHz, mode is 1                                                                                                  No
Frequency      F<freq>;    Set the receive frequency in KHz, if offset is disabled, this is the transmit frequency                                                      No
CDCSS In       G<code>;    <code> must be a valid CDCSS code                                                                                                            No
CDCSS Out      H<code>;    <code> must be a valid CDCSS code                                                                                                            No
Print tones    I           Prints out all configured tones and codes, coma delimited in format: CTCSS In, CTCSS Out, CDCSS In, CDCSS Out                                No
Morse Out      M<text>;    A small buffer for morse code (32 chars)
Power level    P<level>;   Set the power amp level, 0 = lowest, 15 = highest                                                                                           No
Enable Offset  R<state>;   1 turns on repeater offset mode, 0 turns off repeater offset mode                                                                            No
Squelch        S<level>;   Set the squelch level                                                                                                                        No
TX Offset      T<freq>;    The absolute frequency of the repeater offset to transmit on in KHz                                                                          No
Volume         V<level>;   Set the volume level of the receiver                                                                                                         No
Reset          X           Reset all settings to default                                                                                                                No
Sleep          Z           Sleep radio                                                                                                                                  No
Filters        @<state>;   Set bit to enable, clear bit to disable: 0 = pre/de-emphasis, 1 = high pass filter, 2 = low pass filter (default:  ascii 7, all enabled)     No
Vox mode       $<state>;   0 = vox off, >= 1 audio sensitivity. lower value more sensitive                                                                              No
Mic Channel    *<state>;   Set the voice channel. 0 = signal from mic or arduino, 1 = internal tone generator                                                           No
RSSI           ?           Respond with the current receive level in - dBm (no sign provided on numerical response)                                                     No
Tone Gen       % (notes)   To send a tone, use the following format: Single tone: %1,<freq>,<length>; Dual tone: %2,<freq>,<freq>,<length>; DTMF: %3,<key>,<length>;    No
Voice Level    ^           Respond with the current voice level (VSSI)


Responses:

Condition    ASCII      Description
------------ ---------- -----------------------------------------------------------------
Startup      *<code>;   Startup and shield connection status
Success      !;         Generic success message for command that returns no value
Error        X<code>;   Indicates an error code. The numerical value is the type of error
Value        :<value>;  In response to a query
Status       #<value>;  Unsolicited status message
Debug Msg    @<text>;   32 character debug message

*/

#include "HamShield.h"

#define PWM_PIN 3
#define RESET_PIN A3
#define SWITCH_PIN 2

int state;
int txcount = 0;
long timer = 0;
long freq = 144390;
long tx = 0;
char cmdbuff[32] = "";
int temp = 0;
int repeater = 0;
float ctcssin = 0;
float ctcssout = 0;
int cdcssin = 0;
int cdcssout = 0;


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
  
  Serial.begin(115200);
  Serial.print(";;;;;;;;;;;;;;;;;;;;;;;;;;");

  int result = radio.testConnection();
  Serial.print("*");
  Serial.print(result,DEC);
  Serial.print(";");
  radio.initialize(); // initializes automatically for UHF 12.5kHz channel
  Serial.print("*START;");  
  radio.frequency(freq);
  radio.setVolume1(0xF);
  radio.setVolume2(0xF);
  radio.setModeReceive();
  radio.setTxSourceMic();
  radio.setRfPower(0);
  radio.setSQLoThresh(80);
  radio.setSQOn();
}

void loop() {
  
  if(Serial.available()) { 

    int text = Serial.read();

    switch (state) { 

      case 10:
         if(text == 32) { timer = millis();}
         break;

       case 0:
         switch(text) {
           
           case 32:  // space - transmit
               if(repeater == 1) { radio.frequency(tx); } 
               radio.setModeTransmit();
               state = 10;
               Serial.print("#TX,ON;");
               timer = millis();
               break;
           
           case 63: // ? - RSSI
               Serial.print(":");
               Serial.print(radio.readRSSI(),DEC);
               Serial.print(";");
               break;
             
           case 65: // A - CTCSS In
               getValue();
               ctcssin = atof(cmdbuff);
               radio.setCtcss(ctcssin);
               break;
           
           case 66: // B - CTCSS Out
               break;
            
           case 67: // C - CTCSS Enable
               break;
               
           case 68: // D - CDCSS Enable
               break;
               
           case 70: // F - frequency
               getValue();
               freq = atol(cmdbuff);
               if(radio.frequency(freq) == true) { Serial.print("@"); Serial.print(freq,DEC); Serial.print(";!;"); } else { Serial.print("X1;"); } 
               break;

           case 'M':
               getValue();
               radio.setModeTransmit();
               delay(300);
               radio.morseOut(cmdbuff);
               state = 10;
               break; 
               
           case 80: // P - power level
               getValue();
               temp = atol(cmdbuff);
               radio.setRfPower(temp);
               break;
           
           case 82: // R - repeater offset mode
               getValue();
               temp = atol(cmdbuff);
               if(temp == 0) { repeater = 0; }
               if(temp == 1) { repeater = 1; }
               break;
           
           case 83: // S - squelch
               getValue();
               temp = atol(cmdbuff);
               radio.setSQLoThresh(temp);
               break;
           
           case 84: // T - transmit offset 
               getValue();
               tx = atol(cmdbuff);
               break;
      
           
           case 94: // ^ - VSSI (voice) level
               Serial.print(":");
               Serial.print(radio.readVSSI(),DEC); 
               Serial.print(";");
         }
        break;
     }

  }
      if(state == 10) { 
    if(millis() > (timer + 500)) { Serial.print("#TX,OFF;");radio.setModeReceive(); if(repeater == 1) { radio.frequency(freq); }  state = 0; txcount = 0; }
    }
}

void getValue() {
  int p = 0;
  char temp;
  for(;;) {
     if(Serial.available()) { 
        temp = Serial.read();
        if(temp == 59) { cmdbuff[p] = 0; Serial.print("@");
           for(int x = 0; x < 32; x++) {  Serial.print(cmdbuff[x]); }
         return;
        }
        cmdbuff[p] = temp;
        p++;
        if(p == 32) { 
         Serial.print("@");
           for(int x = 0; x < 32; x++) { 
             Serial.print(cmdbuff[x]);
           } 
          
          cmdbuff[0] = 0; 

        Serial.print("X0;"); return; }      // some sort of alignment issue? lets not feed junk into whatever takes this string in
     }
  }
}

