/* Hamshield
 * Example: AX25 Receive
 * This example receives AFSK test data. You will need seperate 
 * AFSK equipment to send data for this example.
 * Connect the HamShield to your Arduino. Screw the antenna 
 * into the HamShield RF jack. Plug a pair of headphones into 
 * the HamShield. Connect the Arduino to wall power and then to 
 * your computer via USB. After uploading this program to your 
 * adruino, open the Serial Monitor so you will see the AFSK 
 * packet. Send AFSK packet from AFSK equipment at 145.01MHz.

 *  Note: add message receive code
 */

#include <HamShield.h>
#include <DDS.h>
#include <AFSK.h>

#define PWM_PIN 3
#define RESET_PIN A3
#define SWITCH_PIN 2

HamShield radio;
DDS dds;
AFSK afsk;

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

  Serial.println(F("Radio test connection"));
  Serial.println(radio.testConnection(), DEC);
  Serial.println(F("Initialize"));
  delay(100);
  radio.initialize();
  radio.frequency(145010);
  radio.setSQOff();
  Serial.println(F("Frequency"));
  delay(100);
  Serial.print(F("Squelch(H/L): "));
  Serial.print(radio.getSQHiThresh());
  Serial.print(F(" / "));
  Serial.println(radio.getSQLoThresh());
  radio.setModeReceive();
  Serial.println(F("DDS Start"));
  delay(100);
  dds.start();
  Serial.println(F("AFSK start"));
  delay(100);
  afsk.start(&dds);
  Serial.println(F("Starting..."));
  delay(100);
  dds.setAmplitude(255);
}

uint32_t last = 0;
void loop() {
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

//TODO: d2 is the switch input, so remove this
ISR(ADC_vect) {
  static uint8_t tcnt = 0;
  TIFR1 = _BV(ICF1); // Clear the timer flag
  //PORTD |= _BV(2); // Diagnostic pin (D2)
  //dds.clockTick();
  afsk.timer();
  //PORTD &= ~(_BV(2)); // Pin D2 off again
}
