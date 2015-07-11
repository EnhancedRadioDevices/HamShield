#define DDS_REFCLK_DEFAULT 9600

#include <HamShield.h>
#include <Wire.h>

HamShield radio;
DDS dds;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);

  Serial.println(F("Radio test connection"));
  Serial.println(radio.testConnection(), DEC);
  Serial.println(F("Initialize"));
  delay(100);
  radio.initialize();
  Serial.println(F("Frequency"));
  delay(100);
  radio.setVHF();
  radio.frequency(145050);
  radio.setRfPower(0);
  Serial.println(F("DDS Start"));
  delay(100);
  dds.start();
  Serial.println(F("AFSK start"));
  delay(100);
  radio.afsk.start(&dds);
  Serial.println(F("Starting..."));
  pinMode(11, INPUT); // Bodge for now, as pin 3 is hotwired to pin 11
  delay(100);
}

void loop() {
  // put your main code here, to run repeatedly:
    AFSK::Packet *packet = AFSK::PacketBuffer::makePacket(22 + 32);
    packet->start();
    packet->appendCallsign("VE6SLP",0);
    packet->appendCallsign("VA6GA",15,true);
    packet->appendFCS(0x03);
    packet->appendFCS(0xf0);
    packet->print(F("Hello "));
    packet->println(millis());
    packet->finish();
    
    bool ret = radio.afsk.putTXPacket(packet);

    if(radio.afsk.txReady()) {
      Serial.println(F("txReady"));
      radio.setModeTransmit();
      //delay(100);
      if(radio.afsk.txStart()) {
        Serial.println(F("txStart"));
      } else {
        radio.setModeReceive();
      }
    }
    // Wait 2 seconds before we send our beacon again.
    Serial.println("tick");
    // Wait up to 2.5 seconds to finish sending, and stop transmitter.
    // TODO: This is hackery.
    for(int i = 0; i < 500; i++) {
      if(radio.afsk.encoder.isDone())
         break;
      delay(50);
    }
    Serial.println("Done sending");
    radio.setModeReceive();
    delay(30000);
}

/*ISR(TIMER2_OVF_vect) {
  TIFR2 = _BV(TOV2);
  static uint8_t tcnt = 0;
  if(++tcnt == 8) {
  digitalWrite(2, HIGH);
  dds.clockTick();
  digitalWrite(2, LOW);
    tcnt = 0;
  }
}*/
ISR(ADC_vect) {
  static uint8_t tcnt = 0;
  TIFR1 = _BV(ICF1); // Clear the timer flag
  PORTD |= _BV(2); // Diagnostic pin (D2)
  dds.clockTick();
  if(++tcnt == 1) {
    if(radio.afsk.encoder.isSending()) {
      radio.afsk.timer();
    }
    tcnt = 0;
  }
  PORTD &= ~(_BV(2)); // Pin D2 off again
}

