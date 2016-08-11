/* 

Indentifier 

Arduino audio overlay example

*/


#include <HamShield.h>

#define DOT 100

#define PWM_PIN 3
#define RESET_PIN A3
#define SWITCH_PIN 2

HamShield radio;

const char *bascii = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?'!/()&:;=+-_\"$@",
  *bitu[] = { ".-","-...","-.-.","-..",".","..-.","--.","....","..",".---","-.-",".-..","--","-.","---",".--.","--.-",".-.","...","-","..-","...-",".--","-..-","-.--","--..","-----",".----","..---","...--","....-",".....","-....","--...","---..","----.",".-.-.-","--..--","..--..",".----.","-.-.--","-..-.","-.--.","-.--.-",".-...","---...","-.-.-.","-...-",".-.-.","-....-","..--.-",".-..-.","...-..-",".--.-."
  };
  
const char *callsign = {"1ZZ9ZZ/B"} ;   
  
char morsebuffer[8];  
  
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
  Serial.println("starting up..");

  Serial.print("Radio status: ");
  int result = radio.testConnection();
  Serial.println(result,DEC);
  radio.initialize();
  radio.frequency(446000);
  radio.setVolume1(0xF);
  radio.setVolume2(0xF);
  radio.setModeReceive();
  radio.setTxSourceMic();
  radio.setSQLoThresh(80);
  radio.setSQOn();
  Serial.println("Done with radio beacon setup. Press and hold a key to transmit.");
}

int state = 0;
long timer = 0;
int morseletter = 0;
int morsesymbol = 0;
long keyer = 0;
char symbol;

void loop() { 
  if(Serial.available() > 0) {
    if(state == 0) { 
      state = 10;
      radio.setModeTransmit();
      timer = millis();
      keyer = millis();
    }
    if(state == 10) { 
      timer = millis();
    }
  }
  if(millis() > (timer + 500)) { radio.setModeReceive(); morseletter = 0; morsesymbol = 0; state = 0; }
  if(state == 10) { 
    if(millis() > (keyer + (DOT * 3))) { 
      keyer = millis();
      symbol = lookup(callsign[morseletter],morsesymbol);
      if(symbol == '-') { tone(9,1000,DOT*3); } 
      if(symbol == '.') { tone(9,1000,DOT); } 
      if(symbol == 0) { morsesymbol = 0; morseletter++; }
      if(callsign[morseletter] == 0) { morsesymbol = 0; morseletter = 0; }  
    }
  }
}

char lookup(char letter, int morsesymbol) { 
 for(int x = 0; x < 54; x++) { 
  if(letter == bascii[x]) { 
    return bitu[x][morsesymbol]; 
  } 
 }
}
