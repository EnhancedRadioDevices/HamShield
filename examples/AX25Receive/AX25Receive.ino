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
//  radio.setVHF();
//  radio.setRfPower(0);
//  radio.setModeReceive();
  radio.setVolume1(0xFF);
  radio.setVolume2(0xFF);
  radio.frequency(145050);
  Serial.println(F("DDS Start"));
  delay(100);
  dds.start();
  Serial.println(F("AFSK start"));
  delay(100);
  radio.afsk.start(&dds);
  Serial.println(F("Starting..."));
  pinMode(11, INPUT); // Bodge for now, as pin 3 is hotwired to pin 11
  delay(100);
  dds.setAmplitude(255);
}

uint32_t last = 0;
void loop() {
   if(radio.afsk.decoder.read() || radio.afsk.rxPacketCount()) {
      // A true return means something was put onto the packet FIFO
      Serial.println("Packet");
      // If we actually have data packets in the buffer, process them all now
      while(radio.afsk.rxPacketCount()) {
        AFSK::Packet *packet = radio.afsk.getRXPacket();
        if(packet) {
          for(unsigned short i = 0; i < packet->len; ++i)
            Serial.write((uint8_t)packet->getByte());
          AFSK::PacketBuffer::freePacket(packet);
        }
      }
    }
    /*if(last < millis()) {
      Serial.println(radio.readRSSI());
      last = millis()+1000;
    }*/
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
  //dds.clockTick();
  radio.afsk.timer();
  PORTD &= ~(_BV(2)); // Pin D2 off again
}

