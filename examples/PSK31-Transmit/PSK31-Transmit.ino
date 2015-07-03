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

const uint8_t amplitudeShape[41] = {
  255, 241, 228, 215, 203, 191, 181, 171, 161, 152, 143, 135, 128, 121, 114, 107, 101, 96, 90, 85, 80, 76, 72, 68, 64, 60, 57, 54, 51, 48, 45, 42, 40, 38, 36, 34, 32, 30, 28, 27, 25
};

// This will trigger at 8kHz
ISR(ADC_vect) {
  static uint8_t tcnt = 0;
  TIFR1 |= _BV(ICF1);
  // Wave shaping
  PORTD |= _BV(2);
  // TODO: Improve how this would perform.
  if(tcnt < 82)
    dds.setAmplitude(amplitudeShape[(82-tcnt)/2]);
  if(tcnt > (255-82))
    dds.setAmplitude(amplitudeShape[(tcnt-173)/2]);
  //else if(tcnt > (255-64))
  //  dds.setAmplitude((255 - tcnt));
  //else dds.setAmplitude(255);
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
