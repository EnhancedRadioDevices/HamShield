/* 

Record sound and then plays it back a few times.
Very low sound quality @ 2KHz 0.75 seconds
A bit robotic and weird

*/

#include <HamShield.h>

#define PWM_PIN 3
#define RESET_PIN A3
#define SWITCH_PIN 2

#define RATE 500
#define SIZE 1500

HamShield radio;

char sound[SIZE];
unsigned int sample1;
int x = -1;
int16_t rssi;
byte mode = 8;

void setup() { 
  // NOTE: if not using PWM out, it should be held low to avoid tx noise
  pinMode(PWM_PIN, OUTPUT);
  digitalWrite(PWM_PIN, LOW);
  
  // prep the switch
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  
  // set up the reset control pin
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, HIGH);
  
  // int result = radio.testConnection();
  radio.initialize();
  radio.setFrequency(446000);
  setPwmFrequency(9, 1);
}



void loop() {
   rssi = radio.readRSSI();
   if(rssi > -100) {
     if(x == -1) { 
       for(x = 0; x < SIZE; x++) {
       if(mode == 4) { 
       sample1 = analogRead(0);
       sound[x] = sample1 >> 4;
       delayMicroseconds(RATE); x++;
       sample1 = analogRead(0); 
       sound[x] = (sample1 & 0xF0) | sound[x];
       delayMicroseconds(RATE);
       } else {
       sound[x] = analogRead(0);
       delayMicroseconds(RATE); x++;
       sound[x] = analogRead(0);
       delayMicroseconds(RATE);
       }
      }
     }
   }
   if(rssi < -100) { 
     if(x == 1500) { 
       radio.setModeTransmit();
       delay(500);
       tone(9,1000,500); delay(750);
       for(int r = 0; r < 10; r++) { 
       for(x = 0; x < SIZE; x++) {
         if(mode == 4) { 

         analogWrite(9,sound[x] << 4);
         delayMicroseconds(RATE); x++;
         analogWrite(9,sound[x] & 0xF);
         delayMicroseconds(RATE); } else { 

         analogWrite(9,sound[x]);
         delayMicroseconds(RATE); x++;
         analogWrite(9,sound[x]);
         delayMicroseconds(RATE);         
         } 
       } }
       tone(9,1000,500); delay(750);
     radio.setModeReceive();
     x = -1;
     }
   }
}

void setPwmFrequency(int pin, int divisor) {
  byte mode;
  if(pin == 5 || pin == 6 || pin == 9 || pin == 10) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 64: mode = 0x03; break;
      case 256: mode = 0x04; break;
      case 1024: mode = 0x05; break;
      default: return;
    }
    if(pin == 5 || pin == 6) {
      TCCR0B = TCCR0B & 0b11111000 | mode;
    } else {
      TCCR1B = TCCR1B & 0b11111000 | mode;
    }
  } else if(pin == 3 || pin == 11) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 32: mode = 0x03; break;
      case 64: mode = 0x04; break;
      case 128: mode = 0x05; break;
      case 256: mode = 0x06; break;
      case 1024: mode = 0x7; break;
      default: return;
    }
    TCCR2B = TCCR2B & 0b11111000 | mode;
  }
}

