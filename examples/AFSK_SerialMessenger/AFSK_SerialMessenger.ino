/* Hamshield
 * Example: AFSK Serial Messenger
 * Serial glue to send messages over APRS. You will need a 
 * seperate AFSK receiver to test the output of this example.
 * Connect the HamShield to your Arduino. Screw the antenna 
 * into the HamShield RF jack. After uploading this program 
 * to your Arduino, open the Serial Monitor to monitor. Type 
 * a message under 254 characters into the bar at the top of 
 * the monitor. Click the "Send" button. Check for output on 
 * AFSK receiver.
 *
 * To send a message: connect to the Arduino over a Serial link.
 * Send the following over the serial link:
 * `from,to,:message
 * example: * KG7OGM,KG7OGM,:Hi there`
 */



#include <HamShield.h>
#include <DDS.h>
#include <packet.h>
#include <avr/wdt.h> 

#define MIC_PIN 3
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
  pinMode(MIC_PIN, OUTPUT);
  digitalWrite(MIC_PIN, LOW);
  
  // prep the switch
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  
  // set up the reset control pin
  pinMode(RESET_PIN, OUTPUT);
  // turn on the radio
  digitalWrite(RESET_PIN, HIGH);
  delay(5); // wait for device to come up
  
  Serial.begin(9600);

  radio.initialize();
  radio.frequency(145570);
  radio.setRfPower(0);
  radio.setVolume1(0xFF);
  radio.setVolume2(0xFF);
  radio.setSQHiThresh(-100);
  radio.setSQLoThresh(-100);
  //radio.setSQOn();
  radio.bypassPreDeEmph();
  dds.start();
  afsk.start(&dds);
  delay(100);
  radio.setModeReceive();
  Serial.println("HELLO");
}

void loop() {
    if(Serial.available()) { 
      char temp = (char)Serial.read();
      if(temp == '`') { 
       //Serial.println(messagebuff);
       prepMessage(); 
       msgptr = 0; 
       Serial.print("!!");
      } 
      else { 
        messagebuff += temp;
        msgptr++;
      }
    }
    if(msgptr > 254) { messagebuff = ""; Serial.print("X!"); }
    
   if(afsk.decoder.read() || afsk.rxPacketCount()) {
      // A true return means something was put onto the packet FIFO
      // If we actually have data packets in the buffer, process them all now
      while(afsk.rxPacketCount()) {
        AFSK::Packet *packet = afsk.getRXPacket();
        Serial.print(F("Packet: "));
        if(packet) {
          packet->printPacket(&Serial);
          AFSK::PacketBuffer::freePacket(packet);
        }
      }
    }
}
    

void prepMessage() { 
   radio.setModeTransmit();
  delay(1000);
  origin_call = messagebuff.substring(0,messagebuff.indexOf(','));                                          // get originating callsign
  destination_call = messagebuff.substring(messagebuff.indexOf(',')+1,messagebuff.indexOf(',',messagebuff.indexOf(',')+1)); // get the destination call
  textmessage = messagebuff.substring(messagebuff.indexOf(":")+1);
  
 // Serial.print("From: "); Serial.print(origin_call); Serial.print(" To: "); Serial.println(destination_call); Serial.println("Text: "); Serial.println(textmessage);

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
  messagebuff = "";
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
    afsk.timer();
    tcnt = 0;
  }
}