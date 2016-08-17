/* Hamshield
 * Example: AFSK Packet Tester
 * This example sends AFSK test data. You will need a seperate 
 * AFSK receiver to test the output of this example.
 * Connect the HamShield to your Arduino. Screw the antenna 
 * into the HamShield RF jack. Connect the Arduino to wall 
 * power and then to your computer via USB. After uploading 
 * this program to your adruino, open the Serial Monitor to 
 * monitor the process of the HamShield. Check for output on 
 * AFSK receiver.

 *  Note: add message receive code
 */

#define DDS_REFCLK_DEFAULT 9600

#include <HamShield.h>
#include <DDS.h>
#include <AFSK.h>
#include <avr/wdt.h> 

#define PWM_PIN 3
#define RESET_PIN A3
#define SWITCH_PIN 2

HamShield radio;
DDS dds;
AFSK afsk;
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
  // turn on pwr to the radio
  digitalWrite(RESET_PIN, HIGH);
  
  Serial.begin(115200);
  
  radio.initialize();
  radio.frequency(144390);
  radio.setRfPower(0);
  dds.start();
  afsk.start(&dds);
  delay(100);
  Serial.println("HELLO");
}

void loop() {
  messagebuff = "KC7IBT,KC7IBT,:HAMSHIELD TEST";
  prepMessage();
  delay(10000);
} 

void prepMessage() { 
  radio.setModeTransmit();
  delay(500);
  origin_call = messagebuff.substring(0,messagebuff.indexOf(','));                                          // get originating callsign
  destination_call = messagebuff.substring(messagebuff.indexOf(',')+1,messagebuff.indexOf(',',messagebuff.indexOf(',')+1)); // get the destination call
  textmessage = messagebuff.substring(messagebuff.indexOf(":")+1);
  
  Serial.print("From: "); Serial.print(origin_call); Serial.print(" To: "); Serial.println(destination_call); Serial.println("Text: "); Serial.println(textmessage);

  AFSK::Packet *packet = AFSK::PacketBuffer::makePacket(22 + 32);

  packet->start();
  packet->appendCallsign(origin_call.c_str(),0);
  packet->appendCallsign(destination_call.c_str(),15,true);   
  packet->appendFCS(0x03);
  packet->appendFCS(0xf0);
  packet->print(textmessage);
  packet->finish();

  bool ret = afsk.putTXPacket(packet);

  if(afsk.txReady()) {
    Serial.println(F("txReady"));
    radio.setModeTransmit();
    //delay(100);
    if(afsk.txStart()) {
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
    if(afsk.encoder.isDone())
       break;
    delay(50);
  }
  Serial.println("Done sending");
  radio.setModeReceive();
}
 
ISR(TIMER2_OVF_vect) {
  TIFR2 = _BV(TOV2);
  static uint8_t tcnt = 0;
  if(++tcnt == 8) {
    dds.clockTick();
    tcnt = 0;
  }
}

ISR(ADC_vect) {
  static uint8_t tcnt = 0;
  TIFR1 = _BV(ICF1); // Clear the timer flag
  dds.clockTick();
  if(++tcnt == 1) {
    if(afsk.encoder.isSending()) {
      afsk.timer();
    }
    tcnt = 0;
  }
}
