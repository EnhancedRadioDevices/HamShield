/* Serial glue to send messages over APRS 
 *  
 *  To do: add message receive code
 *  
 */


#define DDS_REFCLK_DEFAULT 9600

#include <HamShield.h>
#include <avr/wdt.h> 

#define PWM_PIN 3
#define RESET_PIN A3
#define SWITCH_PIN 2

HamShield radio;
DDS dds;
String messagebuff = "";
String origin_call = "";
String destination_call = "";
String textmessage = "";
int msgptr = 0;

void setup() {
  // NOTE: if not using PWM out, it should be held low to avoid tx noise
  pinMode(PWM_PIN, OUTPUT);
  digitalWrite(PWM_PIN, LOW);
  
  // prep the switch
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  
  // set up the reset control pin
  pinMode(RESET_PIN, OUTPUT);
  // turn on the radio
  digitalWrite(RESET_PIN, HIGH);
  
  Serial.begin(115200);

  radio.initialize();
  radio.frequency(145570);
  radio.setRfPower(0);
  dds.start();
  radio.afsk.start(&dds);
  delay(100);
  Serial.println("HELLO");
}

String temp[1] = "";


void loop() {
    if(Serial.available()) { 
      char temp = (char)Serial.read();
      if(temp == '`') { 
       prepMessage(); msgptr = 0; Serial.print("!!"); } 
      else { 
        messagebuff += temp;
        msgptr++;
      }
    }
    if(msgptr > 254) { messagebuff = ""; Serial.print("X!"); } 
}
    

void prepMessage() { 
   radio.setModeTransmit();
   delay(500);
   origin_call = messagebuff.substring(0,messagebuff.indexOf(','));        // get originating callsign
   destination_call = messagebuff.substring(messagebuff.indexOf(',')+1,
              messagebuff.indexOf(',',messagebuff.indexOf(',')+1)); // get the destination call
   textmessage = messagebuff.substring(messagebuff.indexOf(":"));
  
    AFSK::Packet *packet = AFSK::PacketBuffer::makePacket(22 + 32);

    packet->start();
    packet->appendCallsign(origin_call.c_str(),0);
    packet->appendCallsign(destination_call.c_str(),15,true);   
    packet->appendFCS(0x03);
    packet->appendFCS(0xf0);
    packet->print(textmessage);
    packet->finish();

    textmessage = "";
    
    bool ret = radio.afsk.putTXPacket(packet);

    if(radio.afsk.txReady()) {
      Serial.println(F("txReady"));
      //radio.setModeTransmit();
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
    delay(3000);
    radio.setModeReceive();
} 
 

// TODO: d2 is now our switch, so don't write to that
ISR(TIMER2_OVF_vect) {
  TIFR2 = _BV(TOV2);
  static uint8_t tcnt = 0;
  if(++tcnt == 8) {
  //digitalWrite(2, HIGH);
  dds.clockTick();
  //digitalWrite(2, LOW);
    tcnt = 0;
  }
}

ISR(ADC_vect) {
  static uint8_t tcnt = 0;
  TIFR1 = _BV(ICF1); // Clear the timer flag
  //PORTD |= _BV(2); // Diagnostic pin (D2)
  dds.clockTick();
  if(++tcnt == 1) {
    if(radio.afsk.encoder.isSending()) {
      radio.afsk.timer();
    }
    tcnt = 0;
  }
  //PORTD &= ~(_BV(2)); // Pin D2 off again
}


