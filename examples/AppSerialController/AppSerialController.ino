/* Hamshield
 * Example: SerialController
 * This application is used in conjunction with a computer to provide full serial control of HamShield.
*/

#include <HamShield.h>

#define MIC_PIN 3
#define RESET_PIN A3
#define SWITCH_PIN 2

HamShield radio;

uint8_t freq_buffer[32];
uint8_t pl_tx_buffer[32];
uint8_t pl_rx_buffer[32];

void setup() {
  // NOTE: if not using PWM out, it should be held low to avoid tx noise
  pinMode(MIC_PIN, OUTPUT);
  digitalWrite(MIC_PIN, LOW);
  
  // prep the switch
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  
  // set up the reset control pin
  // NOTE: HamShieldMini doesn't have a reset pin, so this has no effect
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, HIGH);
  delay(5); // wait for device to come up
  
  Serial.begin(9600);
  Serial.println("If the sketch freezes at radio status, there is something wrong with power or the shield");
  Serial.print("Radio status: ");
  int result = radio.testConnection();
  Serial.println(result,DEC);
  Serial.println("Setting radio to its defaults..");
  radio.initialize();
  radio.setRfPower(0);
  radio.frequency(432100); // 70cm calling frequency
  radio.setModeReceive();
}

void loop() {
  if(Serial.available()) { 
    uint8_t buf = Serial.read();
    Serial.write(buf);
    switch (buf) {
      case 'X':                               // absorb reset command because we are already reset
        break;
      case 'F':                               // frequency configuration command
        tune_freq();  break;
      case 'P':                               // TX PL Tone configuration command
        pl_tone_tx();  break;
      case 'R':                               // RX PL Tone configuration command
        pl_tone_rx(); break;
      case 'T':                               // turn on transmitter command
        tx_on();  break;
      case 'O':                               // turn off transmitter command
        tx_off();  break;
      case 'A':                               // configure amplifier
        amplifier();  break;
      case 'D':                               // configure predeemph 
        predeemph();  break;
      default:
        break;
    }
  }
}

void tx_on() { 
  radio.setModeTransmit();
  Serial.println("Transmitting");
}

void tx_off() { 
  radio.setModeReceive();
  Serial.println("Transmit off");
}

void pl_tone_tx() { 
  Serial.println("TX PL tone");
  memset(pl_tx_buffer,0,32);
  uint8_t ptr = 0;
  while(1) { 
    if(Serial.available()) { 
    uint8_t buf = Serial.read();
    Serial.write(buf);
    if(buf == 'X') { return; }
    if(buf == '!') { pl_tx_buffer[ptr] = 0; program_pl_tx(); return; }
    if(ptr == 31) { return; }
    pl_tx_buffer[ptr] = buf; ptr++; 
    }
  }
}

void program_pl_tx() { 
  Serial.print("programming TX PL to ");
  for(int x = 0; x < 32; x++) {
  Serial.write(pl_tx_buffer[x]);
  }
  long pl_tx = atof(pl_tx_buffer);
  Serial.print(" Which is FLOAT of ");
  Serial.println(pl_tx,DEC);
  radio.setCtcss(pl_tx);
}

void pl_tone_rx() { 
  Serial.println("RX PL tone");
  memset(pl_rx_buffer,0,32);
  uint8_t ptr = 0;
  while(1) { 
    if(Serial.available()) { 
    uint8_t buf = Serial.read();
    Serial.write(buf);
    if(buf == 'X') { return; }
    if(buf == '!') { pl_rx_buffer[ptr] = 0; program_pl_rx(); return; }
    if(ptr == 31) { return; }
    pl_rx_buffer[ptr] = buf; ptr++; 
    }
  }
}

void program_pl_rx() { 
  Serial.print("programming RX PL to ");
  for(int x = 0; x < 32; x++) {
  Serial.write(pl_rx_buffer[x]);
  }
  long pl_rx = atof(pl_rx_buffer);
  Serial.print(" Which is FLOAT of ");
  Serial.println(pl_rx,DEC);
  radio.setCtcss(pl_rx);
}




void tune_freq() { 
  Serial.println("program frequency mode");
  memset(freq_buffer,0,32);
  uint8_t ptr = 0;
  while(1) {
    if(Serial.available()) { 
       uint8_t buf = Serial.read();
       Serial.write(buf);
       if(buf == 'X') { return; }
       if(buf == '!') { freq_buffer[ptr] = 0; program_frequency(); return; } 
       if(buf != '.') { freq_buffer[ptr] = buf; ptr++; } 
       if(ptr == 31) { return; } 
    }
  }
}

void program_frequency() { 
  Serial.print("programming frequency to ");
  for(int x = 0; x < 32; x++) {
  Serial.write(freq_buffer[x]);
  }
  long freq = atol(freq_buffer);
  Serial.print(" Which is LONG of ");
  Serial.println(freq,DEC);
  radio.frequency(freq);
}


void amplifier() {
   while(1) {
    if(Serial.available()) { 
       uint8_t buf = Serial.read();
       Serial.write(buf);
       if(buf == 'X') { return; }
       if(buf != '!') { radio.setRfPower(buf); return; } 
       if(buf == '!') { return; }
    }
  } 
  
  
  } 

void predeemph() { }


