#include <HamShield.h>
#include <Wire.h>

HamShield radio;
DDS dds;

#define PWM_PIN 3
#define RESET_PIN A3
#define SWITCH_PIN 2

#define DON(p) PORTD |= _BV((p))
#define DOFF(p) PORTD &= ~(_BV((p)))

void setup() {
  Serial.begin(9600);
  Wire.begin();
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  
    // NOTE: if not using PWM out, it should be held low to avoid tx noise
  pinMode(PWM_PIN, OUTPUT);
  digitalWrite(PWM_PIN, LOW);
  
  // prep the switch
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  
  // set up the reset control pin
  pinMode(RESET_PIN, OUTPUT);
  // turn on pwr to the radio
  digitalWrite(RESET_PIN, HIGH);
  
  Serial.println(F("Radio test connection"));
  Serial.println(radio.testConnection(), DEC);
  Serial.println(F("Initialize"));
  delay(100);
  radio.initialize();
  Serial.println(F("Frequency"));
  delay(100);
  radio.frequency(145010);
  //radio.setRfPower(0);
  delay(100);
  dds.start();
  delay(100);
  radio.afsk.start(&dds);
  delay(100);
  dds.setFrequency(0);
  dds.on();
  dds.setAmplitude(255);
  I2Cdev::writeWord(A1846S_DEV_ADDR_SENLOW, 0x44, 0b0000011111111111);
  //I2Cdev::writeWord(A1846S_DEV_ADDR_SENLOW, 0x53, 0x0);
  //I2Cdev::writeWord(A1846S_DEV_ADDR_SENLOW, 0x32, 0xffff);
}

void loop() {
  DON(6);
    AFSK::Packet *packet = AFSK::PacketBuffer::makePacket(22 + 32);
    packet->start();
    packet->appendCallsign("VE6SLP",0);
    packet->appendCallsign("VA6GA",15,true);
    packet->appendFCS(0x03);
    packet->appendFCS(0xf0);
    packet->print(F("Hello "));
    packet->print(millis());
    packet->println(F("\r\nThis is a test of the HamShield Kickstarter prototype. de VE6SLP"));
    packet->finish();
    
    bool ret = radio.afsk.putTXPacket(packet);

    if(radio.afsk.txReady()) {
      Serial.println(F("txReady"));
      radio.setModeTransmit();
      if(radio.afsk.txStart()) {
        Serial.println(F("txStart"));
      } else {
        Serial.println(F("Tx Start failure"));
        radio.setModeReceive();
      }
    }
    // Wait 2 seconds before we send our beacon again.
    Serial.println("tick");
    // Wait up to 2.5 seconds to finish sending, and stop transmitter.
    // TODO: This is hackery.
    DOFF(6);
    for(int i = 0; i < 500; i++) {
      if(radio.afsk.encoder.isDone())
         break;
      delay(50);
      Serial.println("Not done");
    }
    Serial.println("Done sending");
    delay(100);
    radio.setModeReceive();
    delay(2000);
}

ISR(ADC_vect) {
  TIFR1 = _BV(ICF1); // Clear the timer flag
  DON(4);
  dds.clockTick();
  DON(5);
  radio.afsk.timer();
  DOFF(5);
  DOFF(4);
}
