/* Just Transmit */

#include <HamShield.h>
#include <Wire.h>

HamShield radio;

void setup() {
  Wire.begin();
  Serial.begin(9600);
  Serial.println("If the sketch freezes at radio status, there is something wrong with power or the shield");
  Serial.print("Radio status: ");
  int result = radio.testConnection();
  Serial.println(result,DEC);
  Serial.println("Setting radio to its defaults..");
  radio.initialize();
  radio.setRfPower(15);
  radio.setChanMode(3);
}

void loop() {
  radio.bypassPreDeEmph();
  radio.frequency(144000);
  // radio.setTxSourceNone();
  radio.setModeTransmit();
  for(;;) { }
}

