/* Hamshield
 * Example: KISS
 * This is a example configures the HamShield to be used as 
 * a TNC/KISS device. You will need a KISS device to input 
 * commands to the HamShield
 * Connect the HamShield to your Arduino. Screw the antenna 
 * into the HamShield RF jack. Connect the Arduino to wall 
 * power and then to your computer via USB. Issue commands 
 * via the KISS equipment.
 * 
 * You can also just use the serial terminal to send and receive
 * APRS packets, but keep in mind that several fields in the packet
 * are bit-shifted from standard ASCII (so if you're receiving,
 * you won't get human readable callsigns or paths).
 *
 * To use the KISS example with YAAC:
 * 1. open the configure YAAC wizard
 * 2. follow the prompts and enter in your details until you get to the "Add and Configure Interfaces" window
 * 3. Choose "Add Serial KISS TNC Port"
 * 4. Choose the COM port for your Arduino
 * 5. set baud rate to 9600 (default)
 * 6. set it to KISS-only: with no command to enter KISS mode (just leave the box empty)
 * 7. Use APRS protocol (default)
 * 8. hit the next button and follow directions to finish configuration
*/

#include <HamShield.h>
#include <KISS.h>
#include <DDS.h>
#include <packet.h>
#include <avr/wdt.h>

HamShield radio;
DDS dds;
AFSK afsk;
KISS kiss(&Serial, &radio, &dds, &afsk);

#define PWM_PIN 3
#define RESET_PIN A3
#define SWITCH_PIN 2

void setup() {
  // NOTE: if not using PWM out, it should be held low to avoid tx noise
  pinMode(PWM_PIN, OUTPUT);
  digitalWrite(PWM_PIN, LOW);
  
  // prep the switch
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  
  // set up the reset control pin
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, HIGH);
  delay(5); // wait for device to come up
  
  Serial.begin(9600);
  
  radio.initialize();
  //radio.setSQOff();
  radio.setVolume1(0xFF);
  radio.setVolume2(0xFF);
  radio.setSQHiThresh(-100);
  radio.setSQLoThresh(-100);
  //radio.setSQOn();
  radio.frequency(144390);
  radio.bypassPreDeEmph();

  dds.start();
  afsk.start(&dds);
  delay(100);
  radio.setModeReceive();
}

void loop() {
  kiss.loop();
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