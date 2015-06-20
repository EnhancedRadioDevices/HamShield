/* 

Gauges

Simple gauges for the radio receiver.


*/

#include <HAMShield.h>
#include <Wire.h>

HAMShield radio;

void clr() { 
/* Serial.write(27);
  Serial.print("[2J");     // cursor to home command  */
 Serial.write(27);
  Serial.print("[H");     // cursor to home command 
}

void setup() { 
  analogReference(DEFAULT);
  Serial.begin(115200);
  Wire.begin();
  Serial.print("Radio status: ");
  int result = radio.testConnection();
  Serial.println(result,DEC); 
  radio.initialize();
  radio.setFrequency(446000);
  Serial.println("Entering gauges...");
  tone(9,1000);
  delay(2000);
}

int gauge;
int x = 0;
int y = 0;
int peak = 0;
int a = 0;
int mini = 0;
int vpeak = 0;
int txc = 0;
int mode = 0;

void loop() {
   clr();
   int16_t rssi = radio.readRSSI();
   gauge = map(rssi,-123,-50,0,8);
   Serial.print("[");
   for(x = 0; x < gauge; x++) { 
     Serial.print(".");
   }
   Serial.print("|");
   for(y = x; y < 8; y++) { 
    Serial.print(".");
   }
   Serial.print("] ");
   Serial.print(rssi);
   Serial.println("       ");
   Serial.println("Signal       \n");
   
   // radio.setModeTransmit();
   int16_t vssi = radio.readVSSI();
   // radio.setModeReceive();
   if(vssi > vpeak) { vpeak = vssi; } 
   gauge = map(vssi,-50,-150,0,8);
   Serial.print("[");
   for(x = 0; x < gauge; x++) { 
     Serial.print(".");
   }
   Serial.print("|");
   for(y = x; y < 8; y++) { 
    Serial.print(".");
   }
   Serial.print("] ");
   Serial.print(vpeak);
   Serial.println("       ");
   Serial.println("Audio In\n");   
 
   a = analogRead(0);
   if(a > peak) { peak = a; } 
   if(a < mini) { mini = a; } 
   gauge = map(a,400,1023,0,8);
   Serial.print("[");
   for(x = 0; x < gauge; x++) { 
     Serial.print(".");
   }
   Serial.print("|");
   for(y = x; y < 8; y++) { 
    Serial.print(".");
   }
   Serial.print("] ");
   Serial.print(a,DEC);
   Serial.print(" ("); Serial.print(peak,DEC); Serial.println(")   ");
   Serial.println("Audio RX ADC Peak\n");
}


