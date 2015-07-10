#define DDS_REFCLK_DEFAULT 9600

#include <HamShield.h>
#include <Wire.h>
//#include <LiquidCrystal_I2C.h>

//LiquidCrystal_I2C lcd(0x27,16,2);

HamShield radio;
DDS dds;
volatile uint8_t adcMax=0, adcMin=255;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  /*lcd.init();
  lcd.setCursor(0,0);
  lcd.print(F("RSSI:"));
  lcd.setCursor(0,1);
  lcd.print(F("ADC:"));*/
  Serial.println(F("Radio test connection"));
  Serial.println(radio.testConnection(), DEC);
  Serial.println(F("Initialize"));
  delay(100);
  radio.initialize();
  radio.frequency(144390);
  radio.setVHF();
  radio.setSQOff();
  I2Cdev::writeWord(A1846S_DEV_ADDR_SENLOW, 0x30, 0x06);
  I2Cdev::writeWord(A1846S_DEV_ADDR_SENLOW, 0x30, 0x26);
  I2Cdev::writeWord(A1846S_DEV_ADDR_SENLOW, 0x44, 0b11111111);
  Serial.println(F("Frequency"));
  delay(100);
  Serial.print(F("Squelch(H/L): "));
  Serial.print(radio.getSQHiThresh());
  Serial.print(F(" / "));
  Serial.println(radio.getSQLoThresh());
  radio.setModeReceive();
  Serial.print(F("RX? "));
  Serial.println(radio.getRX());
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
  //lcd.backlight();
}

uint32_t last = 0;
void loop() {
   if(radio.afsk.decoder.read() || radio.afsk.rxPacketCount()) {
      // A true return means something was put onto the packet FIFO
      // If we actually have data packets in the buffer, process them all now
      while(radio.afsk.rxPacketCount()) {
        AFSK::Packet *packet = radio.afsk.getRXPacket();
        Serial.print(F("Packet: "));
        if(packet) {
          packet->print(&Serial);
          AFSK::PacketBuffer::freePacket(packet);
        }
      }
      Serial.println("");
    }
/*    if(last < millis()) {
      uint16_t buf;
      lcd.setCursor(6,0);
      lcd.print(radio.readRSSI());
      lcd.print(F("  "));
      lcd.setCursor(6,1);
      lcd.print(adcMax);
      lcd.print(F(" / "));
      lcd.print(adcMin);
      lcd.print(F("  "));
      lcd.setCursor(11,0);
      lcd.print(radio.afsk.decoder.isReceiving());
      adcMin=255; adcMax=0;
      last = millis()+100;
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
  if(ADCH>adcMax) {
    adcMax = ADCH;
  }
  if(ADCH<adcMin) {
    adcMin = ADCH;
  }
  PORTD &= ~(_BV(2)); // Pin D2 off again
}

