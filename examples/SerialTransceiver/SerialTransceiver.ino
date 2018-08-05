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

Commands:

Mode           ASCII       Description                                                                                                                                  Implemented
-------------- ----------- -------------------------------------------------------------------------------------------------------------------------------------------- -----------------
Transmit       space       Space must be received at least every 500 mS                                                                                                 Yes
Receive        not space   If space is not received and/or 500 mS timeout of space occurs, unit will go into receive mode                                               Yes
Bandwidth      E<mode>;    for 12.5KHz mode is 0, for 25KHz, mode is 1                                                                                                  No
Frequency      F<freq>;    Set the receive frequency in KHz, if offset is disabled, this is the transmit frequency                                                      No
Morse Out      M<text>;    A small buffer for morse code (32 chars)
Power level    P<level>;   Set the power amp level, 0 = lowest, 15 = highest                                                                                           No
Enable Offset  R<state>;   1 turns on repeater offset mode, 0 turns off repeater offset mode                                                                            No
Squelch        S<level>;   Set the squelch level                                                                                                                        No
TX Offset      T<freq>;    The absolute frequency of the repeater offset to transmit on in KHz                                                                          No
RSSI           ?;          Respond with the current receive level in - dBm (no sign provided on numerical response)                                                     No
Voice Level    ^;          Respond with the current voice level (VSSI)


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
  delay(5); // wait for device to come up
  
  Serial.begin(9600);
  Serial.println(";;;;;;;;;;;;;;;;;;;;;;;;;;");

  int result = radio.testConnection();
  Serial.print("*");
  Serial.print(result,DEC);
  Serial.println(";");
  radio.initialize(); // initializes automatically for UHF 12.5kHz channel
  Serial.println("*START;");  
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
               Serial.println("#TX,ON;");
               timer = millis();
               break;
           
           case 63: // ? - RSSI
               Serial.print(":");
               Serial.print(radio.readRSSI(),DEC);
               Serial.println(";");
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
               if(radio.frequency(freq) == true) { Serial.print("@"); Serial.print(freq,DEC); Serial.println(";!;"); } else { Serial.println("X1;"); } 
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
               Serial.println(";");
         }
        break;
     }

  }
      if(state == 10) { 
    if(millis() > (timer + 500)) { Serial.println("#TX,OFF;");radio.setModeReceive(); if(repeater == 1) { radio.frequency(freq); }  state = 0; txcount = 0; }
    }
}

void getValue() {
  int p = 0;
  char temp;
  for(;;) {
     if(Serial.available()) { 
        temp = Serial.read();
        if(temp == 59) { cmdbuff[p] = 0; Serial.print("@");
           for(int x = 0; x < 32; x++) {  Serial.print(cmdbuff[x]);}
		   Serial.println();
         return;
        }
        cmdbuff[p] = temp;
        p++;
        if(p == 32) { 
         Serial.print("@");
           for(int x = 0; x < 32; x++) { 
             Serial.println(cmdbuff[x]);
           } 
          
          cmdbuff[0] = 0; 

        Serial.println("X0;"); return; }      // some sort of alignment issue? lets not feed junk into whatever takes this string in
     }
  }
}

