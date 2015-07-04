#define DDS_REFCLK_DEFAULT 19200
#define DDS_REFCLK_OFFSET  0
#define DDS_DEBUG_SERIAL

#include <HamShield.h>
#include <Wire.h>

HamShield radio;
DDS dds;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  radio.initialize();
  radio.setVHF();
  radio.setRfPower(0);
  radio.setFrequency(145050);
  
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(11, INPUT);
  
  dds.start();
  dds.setFrequency(1200);
  dds.on();
  
  radio.bypassPreDeEmph();
}

enum Sets {
  SET_REF,
  SET_TONE,
  SET_AMPLITUDE
} setting = SET_TONE;

char freqBuffer[8];
char *freqBufferPtr = freqBuffer;
uint16_t lastFreq = 1200;

void loop() {
  while(Serial.available()) {
    char c = Serial.read();
    Serial.print(c);
    switch(c) {
      case 'h':
        Serial.println(F("Commands:"));
        Serial.println(F("RefClk: u = +10, U = +100, r XXXX = XXXX"));
        Serial.println(F("        d = -10, D = -100"));
        Serial.println(F("Radio:  T = transmit, R = receive"));
        Serial.println(F("Tone:   t XXXX = XXXX Hz"));
        Serial.println(F("Amp.:   a XXX  = XXX out of 255"));
        Serial.println(F("DDS:    o = On, O = Off"));
        Serial.println(F("ie. a 31 = 32/255 amplitude, r38400 sets 38400Hz refclk"));
        Serial.println("> ");
        break;
      case 'u':
        dds.setReferenceClock(dds.getReferenceClock()+10);
        dds.setFrequency(lastFreq);
        dds.start();
        Serial.println(F("RefClk + 10 = "));
        Serial.println(dds.getReferenceClock());
        Serial.println("> ");
        break;
      case 'U':
        dds.setReferenceClock(dds.getReferenceClock()+100);
        dds.setFrequency(lastFreq);
        dds.start();
        Serial.println(F("RefClk + 100 = "));
        Serial.println(dds.getReferenceClock());
        Serial.println("> ");
        break;
      case 'd':
        dds.setReferenceClock(dds.getReferenceClock()-10);
        dds.setFrequency(lastFreq);
        dds.start();
        Serial.println(F("RefClk - 10 = "));
        Serial.println(dds.getReferenceClock());
        Serial.println("> ");
        break;
      case 'D':
        dds.setReferenceClock(dds.getReferenceClock()-100);
        dds.setFrequency(lastFreq);
        dds.start();
        Serial.println(F("RefClk - 100 = "));
        Serial.println(dds.getReferenceClock());
        Serial.println("> ");
        break;
      case 'T':
        Serial.println(F("Radio transmit"));
        radio.setModeTransmit();
        Serial.println("> ");
        break;
      case 'R':
        Serial.println(F("Radio receive"));
        radio.setModeReceive();
        Serial.println("> ");
        break;
      case 'r':
        setting = SET_REF;
        break;
      case 't':
        setting = SET_TONE;
        break;
      case 'a':
        setting = SET_AMPLITUDE;
        break;
      case 'o':
        dds.on();
        Serial.println("> ");
        break;
      case 'O':
        dds.off();
        Serial.println("> ");
        break;
      default:
        if(c >= '0' && c <= '9') {
          *freqBufferPtr = c;
          freqBufferPtr++;
        }
        if(c == '\n' && freqBufferPtr != freqBuffer) {
          *freqBufferPtr = '\0';
          freqBufferPtr = freqBuffer;
          uint16_t freq = atoi(freqBuffer);
          if(setting == SET_REF) {
            dds.setReferenceClock(freq);
            dds.setFrequency(lastFreq);
            dds.start();
            Serial.print(F("New Reference Clock: "));
            Serial.println(dds.getReferenceClock());
          } else if(setting == SET_TONE) {
            dds.setFrequency(freq);
            lastFreq = freq;
            Serial.print(F("New Tone: "));
            Serial.println(freq);
          } else if(setting == SET_AMPLITUDE) {
            dds.setAmplitude((uint8_t)(freq&0xFF));
            Serial.print(F("New Amplitude: "));
            Serial.println((uint8_t)(freq&0xFF));
          }
          Serial.println("> ");
        }
        break;
    }
  }
}

ISR(ADC_vect) {
  TIFR1 = _BV(ICF1);
  PORTD |= _BV(2);
  dds.clockTick();
  PORTD &= ~_BV(2);
}
