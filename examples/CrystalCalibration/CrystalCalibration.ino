#define DDS_REFCLK_DEFAULT 38400
#define DDS_REFCLK_OFFSET  0
#define DDS_DEBUG_SERIAL

#include <HamShield.h>

#define PWM_PIN 3
#define RESET_PIN A3
#define SWITCH_PIN 2

HamShield radio;
DDS dds;

void setup() {
  // NOTE: if not using PWM out, it should be held low to avoid tx noise
  pinMode(PWM_PIN, OUTPUT);
  digitalWrite(PWM_PIN, LOW);
  
  // prep the switch
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  
  // set up the reset control pin
  pinMode(RESET_PIN, OUTPUT);
  // turn on radio
  digitalWrite(RESET_PIN, HIGH);
  
  Serial.begin(9600);

  radio.initialize();
  radio.setRfPower(0);
  radio.setFrequency(145050);
  
  dds.start();
  dds.setFrequency(1200);
  dds.on();
  
  radio.bypassPreDeEmph();
}

enum Sets {
  SET_REF,
  SET_TONE,
  SET_AMPLITUDE,
  SET_ADC_HALF,
  SET_OFFSET
} setting = SET_TONE;

char freqBuffer[8];
char *freqBufferPtr = freqBuffer;
uint16_t lastFreq = 1200;

volatile uint16_t recordedPulseLength;
volatile bool recordedPulse = false;
volatile bool listening = false;
volatile uint8_t maxADC = 0, minADC = 255, adcHalf = 40;

void loop() {
  static uint16_t samples = 0;
  static uint16_t pulse;
  static uint32_t lastOutput = 0;
  static float pulseFloat = 0.0;
  if(recordedPulse) {
    uint32_t pulseAveraging;
    uint16_t tmpPulse;
    cli();
    recordedPulse = false;
    tmpPulse = recordedPulseLength;
    sei();
    if(samples++ == 0) {
      pulse = tmpPulse;
      //pulseFloat = tmpPulse;
    } else {
      pulseAveraging = (pulse + tmpPulse) >> 1;
      pulse = pulseAveraging;
      pulseFloat = pulseFloat + 0.01*((float)pulse-pulseFloat);
    }
    if((lastOutput + 1000) < millis()) {
      Serial.print(F("Pulse:   "));
      Serial.println(pulse);
      Serial.print(F("Last:    "));
      Serial.println(tmpPulse);
      Serial.print(F("Samples: "));
      Serial.println(samples);
      Serial.print(F("ADC M/M: "));
      Serial.print(minADC); minADC = 255;
      Serial.print(F(" / "));
      Serial.println(maxADC); maxADC = 0;
      Serial.print(F("Freq:    "));
      // F = 1/(pulse*(1/ref))
      // F = ref/pulse
      Serial.print((float)((float)dds.getReferenceClock()+(float)dds.getReferenceOffset())/(float)pulse);
      Serial.print(F(" / "));
      Serial.print((float)((float)dds.getReferenceClock()+(float)dds.getReferenceOffset())/pulseFloat);
      Serial.print(F(" / "));
      Serial.println(pulseFloat);
      Serial.print(F("Freq2:   "));
      // F = 1/(pulse*(1/ref))
      // F = ref/pulse
      Serial.print((float)dds.getReferenceClock()/(float)pulse);
      Serial.print(F(" / "));
      Serial.println((float)dds.getReferenceClock()/pulseFloat);
      samples = 0;
      lastOutput = millis();
    }
  }
  while(Serial.available()) {
    char c = Serial.read();
    Serial.print(c);
    switch(c) {
      case 'h':
        Serial.println(F("Commands:"));
        Serial.println(F("RefClk: u = +10, U = +100, r XXXX = XXXX"));
        Serial.println(F("        d = -10, D = -100"));
        Serial.println(F("Offset: s XXX = Set refclk offset"));
        Serial.println(F("Radio:  T = transmit, R = receive"));
        Serial.println(F("Tone:   t XXXX = XXXX Hz"));
        Serial.println(F("Amp.:   a XXX  = XXX out of 255"));
        Serial.println(F("DDS:    o = On, O = Off"));
        Serial.println(F("Input:  l = Determine received frequency, L = stop"));
        Serial.println(F("ADC:    m XXX = Set ADC midpoint (zero crossing level)"));
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
      case 'l':
        Serial.println(F("Start frequency listening, DDS off"));
        dds.off();
        listening = true;
        lastOutput = millis();
        Serial.println("> ");
        break;
      case 'L':
        Serial.println(F("Stop frequency listening, DDS on"));
        listening = false;
        samples = 0;
        dds.on();
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
      case 'm':
        setting = SET_ADC_HALF;
        break;
      case 's':
        setting = SET_OFFSET;
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
        if(c == '-' || (c >= '0' && c <= '9')) {
          *freqBufferPtr = c;
          freqBufferPtr++;
        }
        if((c == '\n' || c == '\r') && freqBufferPtr != freqBuffer) {
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
          } else if(setting == SET_ADC_HALF) {
            adcHalf = freq&0xFF;
            Serial.print(F("ADC midpoint set to "));
            Serial.println((uint8_t)(freq&0xFF));
          } else if(setting == SET_OFFSET) {
            dds.setReferenceOffset((int16_t)atoi(freqBuffer));
            dds.setFrequency(lastFreq);
            Serial.print(F("Refclk offset: "));
            Serial.println(dds.getReferenceOffset());
          }
          Serial.println("> ");
        }
        break;
    }
  }
}

ISR(ADC_vect) {
  static uint16_t pulseLength = 0;
  static uint8_t lastADC = 127;
  cli();
  TIFR1 = _BV(ICF1);
  //PORTD |= _BV(2);
  dds.clockTick();
  sei();
  if(listening) {
    pulseLength++;
    if(ADCH >= adcHalf && lastADC < adcHalf) {
      // Zero crossing, upward
      recordedPulseLength = pulseLength;
      recordedPulse = true;
      pulseLength = 0;
    }
    if(minADC > ADCH) {
      minADC = ADCH;
    }
    if(maxADC < ADCH) {
      maxADC = ADCH;
    }
    lastADC = ADCH;
  }
  //PORTD &= ~_BV(2);
}
