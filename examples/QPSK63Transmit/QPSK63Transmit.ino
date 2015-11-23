#include <HamShield.h>
#include "varicode.h"

#define PWM_PIN 3
#define RESET_PIN A3
#define SWITCH_PIN 2

DDS dds;

void setup() {
  // NOTE: if not using PWM out, it should be held low to avoid tx noise
  pinMode(PWM_PIN, OUTPUT);
  digitalWrite(PWM_PIN, LOW);
  
  // prep the switch
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  
  // set up the reset control pin
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, LOW);
  
  
  Serial.begin(9600);
  // put your setup code here, to run once:
  dds.setReferenceClock(32000);
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
    //Serial.println(string[i]);
  }
}

const uint8_t amplitudeShape[41] = {
  255, 241, 228, 215, 203, 191, 181, 171, 161, 152, 143, 135, 128, 121, 114, 107, 101, 96, 90, 85, 80, 76, 72, 68, 64, 60, 57, 54, 51, 48, 45, 42, 40, 38, 36, 34, 32, 30, 28, 27, 25
};

// This will trigger at 8kHz
const uint16_t qpskConvolution[32] = {
  180, 90, -90, 0, -90, 0, 180, 90,
  0, -90, 90, 180, 90, 180, 0, -90,
  90, 180, 0, -90, 0, -90, 90, 180,
  -90, 0, 180, 90, 180, 90, -90, 0
};
uint8_t last5Bits = 0b00000;
ISR(ADC_vect) {
  static uint8_t outer = 0;
  static uint8_t tcnt = 0;
  TIFR1 |= _BV(ICF1);
  // Wave shaping
  // TODO: Improve how this would perform.
  if(tcnt < 81)
    dds.setAmplitude(amplitudeShape[(81-tcnt)/2]);
  if(tcnt > (255-81))
    dds.setAmplitude(amplitudeShape[(tcnt-174)/2]);
  dds.clockTick();

  if(outer++ == 1) {
    outer = 0;
  } else {
    return;
  }
  
  if(tcnt++ == 0) { // Next bit
    last5Bits <<= 1;
    if(!sent) {
      if((bitsToSend & 0x8000) == 0) {
        zeroCount++;
      } else {
        zeroCount = 0;
        last5Bits |= 1;
      }
      dds.changePhaseDeg(qpskConvolution[last5Bits&31]);
      bitsToSend<<=1;
      if(zeroCount == 2) {
        sent = true;
      }
    } else {
      // Idle on zeroes
      dds.changePhaseDeg(qpskConvolution[last5Bits&31]);
    }
  }
}
