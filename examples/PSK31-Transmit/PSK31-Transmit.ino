#include <HamShield.h>
#include "varicode.h"

DDS dds;

void setup() {
  Serial.begin(9600);
  pinMode(11, OUTPUT);
  pinMode(2, OUTPUT);
  // put your setup code here, to run once:
  dds.setReferenceClock(8000);
  dds.start();
  dds.setFrequency(1000);
  dds.on();
}

volatile bool sent = true;
volatile uint16_t bitsToSend = 0;
volatile uint8_t zeroCount = 0;

void sendChar(uint8_t c) {
  uint16_t bits = varicode[c];
  while((bits&0x8000)==0) {
    bits<<=1;
  }
  while(!sent) {} //delay(32);
  cli();
  sent = false;
  bitsToSend = bits;
  sei();
  while(!sent) {} //delay(32);
  //PORTD &= ~_BV(2); // Diagnostic pin (D2)
}

char *string = "Why hello there, friend. Nice to meet you. Welcome to PSK31. 73, VE6SLP sk\r\n";
void loop() {
  int i;
  // put your main code here, to run repeatedly:
  //for(i = 0; i<5; i++)
  //  sendChar(0);
  //  return;
  for(i = 0; i < strlen(string); i++) {
    sendChar(string[i]);
    Serial.println(string[i]);
  }
}

// This will trigger at 8kHz
ISR(ADC_vect) {
  static uint8_t tcnt = 0;
  TIFR1 |= _BV(ICF1);
  // Wave shaping
  PORTD |= _BV(2);
  if(tcnt < 32)
    dds.setAmplitude(tcnt<<3);
  if(tcnt > (255-32))
    dds.setAmplitude((255 - tcnt)<<3);
  dds.clockTick();
  if(tcnt++ == 0) { // Next bit
    //PORTD ^= _BV(2); // Diagnostic pin (D2)
    if(!sent) {
      if((bitsToSend & 0x8000) == 0) {
        zeroCount++;
        dds.changePhaseDeg(+180);
      } else {
        zeroCount = 0;
      }
      bitsToSend<<=1;
      if(zeroCount == 2) {
        sent = true;
      }
    } else {
      // Idle on zeroes
      dds.changePhaseDeg(+180);
    }
  }
  PORTD &= ~_BV(2);
}
