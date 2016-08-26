/* Hamshield
 * Example: Gauges
 * This example prints Signal, Audio In, and Audio Rx ADC 
 * Peak strength to the Serial Monitor in a graphical manner.
 * Connect the HamShield to your Arduino. Screw the antenna 
 * into the HamShield RF jack. Plug a pair of headphones into 
 * the HamShield. Connect the Arduino to wall power and then 
 * to your computer via USB. After uploading this program to 
 * your Arduino, open the Serial Monitor. You will see a 
 * repeating display of different signal strengths. Ex: 
 *
 * [....|....] -73       
 * Signal 
 *
 * Uncheck the "Autoscroll" box at the bottom of the Serial 
 * Monitor to manually control the view of the Serial Monitor.
*/

#include <HamShield.h>

#define PWM_PIN 3
#define RESET_PIN A3
#define SWITCH_PIN 2

HamShield radio;

void setup() {
  // NOTE: if not using PWM out, it should be held low to avoid tx noise
  pinMode(PWM_PIN, OUTPUT);
  digitalWrite(PWM_PIN, LOW);
  
  // prep the switch
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  
  // set up the reset control pin
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, HIGH);
  
  analogReference(DEFAULT);
  Serial.begin(9600);

  Serial.print("Radio status: ");
  int result = radio.testConnection();
  Serial.println(result,DEC); 
  radio.initialize();
  radio.frequency(446000);
  radio.setModeReceive();
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


