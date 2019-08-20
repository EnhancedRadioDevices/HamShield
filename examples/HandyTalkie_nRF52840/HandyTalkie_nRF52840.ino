/* Hamshield
 * Example: HandyTalkie_nRF52840
 * This is a simple example to demonstrate the HamShield working 
 * with an Adafruit Feather nRF52840 Express
 * 
 * HamShield to Feather Connections:
 *  SPKR - Feather A0
 *  MIC - Feather D11
 *  CLK - Feather D5
 *  nCS - Feather D6
 *  DAT - Feather D9
 *  GND - Feather GND
 *  VCC - Feather 3.3V
 *  
 * Connect the HamShield to your Feather as above. 
 * Screw the antenna into the HamShield RF jack. Plug a pair 
 * of headphones into the HamShield.
 * 
 * Connect the Feather nRF52840 Express to your computer via 
 * a USB Micro B cable. After uploading this program to 
 * your Feather, open the Serial Monitor. You should see some
 * text displayed that documents the setup process.
 * 
 * Once the Feather is set up and talking to the HamShield,
 * you can control it over USB-Serial or BLE-Serial(UART).
 * 
 * Try using Adafruit's Bluefruit app to connect to the Feather.
 * Once you're connected, you can control the HamShield using
 * the same commands you'd use over USB-Serial. The response to
 * all commands will be echoed to both USB-Serial and BLE-Serial(UART).
 * 
 * Serial UART commands:
 * t - change from Tx to Rx (or vice versa)
 * F123400 - set frequency to 123400 kHz
*/

#include <bluefruit.h>

// BLE Service
BLEDis  bledis;  // device information
BLEUart bleuart; // uart over ble
BLEBas  blebas;  // battery

#include <HamShield.h>
// create object for radio
HamShield radio(9,5,6);
// To use non-standard pins, use the following initialization
//HamShield radio(ncs_pin, clk_pin, dat_pin);

#define LED_PIN 3
#define RSSI_REPORT_RATE_MS 5000

#define MIC_PIN A1

bool blinkState = false;
bool currently_tx;

uint32_t freq;

unsigned long rssi_timeout;

void setup() {

  // NOTE: if not using PWM out, it should be held low to avoid tx noise
  pinMode(MIC_PIN, OUTPUT);
  digitalWrite(MIC_PIN, LOW);
  
  // initialize serial communication
  Serial.begin(115200);
  while (!Serial) delay(10);
  Serial.println("Setting up BLE");

  // Setup the BLE LED to be enabled on CONNECT
  // Note: This is actually the default behaviour, but provided
  // here in case you want to control this LED manually via PIN 19
  Bluefruit.autoConnLed(true);

  // Config the peripheral connection with maximum bandwidth 
  // more SRAM required by SoftDevice
  // Note: All config***() function must be called before begin()
  Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);

  Bluefruit.begin();
  // Set max power. Accepted values are: -40, -30, -20, -16, -12, -8, -4, 0, 4
  Bluefruit.setTxPower(4);
  Bluefruit.setName("MyBlueHam");
  //Bluefruit.setName(getMcuUniqueID()); // useful testing with multiple central connections
  Bluefruit.setConnectCallback(connect_callback);
  Bluefruit.setDisconnectCallback(disconnect_callback);

  // Configure and Start Device Information Service
  bledis.setManufacturer("Enhanced Radio Devices");
  bledis.setModel("BlueHam");
  bledis.begin();

  // Configure and Start BLE Uart Service
  bleuart.begin();

  // Start BLE Battery Service
  blebas.begin();
  blebas.write(100);

  // Set up and start advertising
  startAdv();
    
  delay(100);  

  Serial.println("beginning Ham radio setup");

  // verify connection
  Serial.println("Testing device connections...");
  if (radio.testConnection()) {
    Serial.println("HamShield connection successful");
  } else {
    Serial.print("HamShield connection failed");
    while(1) delay(100);
  }

  // initialize device
  Serial.println("Initializing radio device...");
  radio.initialize(); // initializes automatically for UHF 12.5kHz channel

  Serial.println("setting default Radio configuration");

  // set frequency
  Serial.println("changing frequency");
  
  radio.setSQOff();
  freq = 432100; // 70cm calling frequency
  radio.frequency(freq);
  
  // set to receive
  
  radio.setModeReceive();
  currently_tx = false;
  Serial.print("config register is: ");
  Serial.println(radio.readCtlReg());
  Serial.println(radio.readRSSI());
  
/*
  // set to transmit
  radio.setModeTransmit();
  // maybe set PA bias voltage
  Serial.println("configured for transmit");
  radio.setTxSourceMic();
  
  
  */  
  radio.setRfPower(0);
    
  // configure Arduino LED for
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  rssi_timeout = 0;

}

void startAdv(void)
{
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();

  // Include bleuart 128-bit uuid
  Bluefruit.Advertising.addService(bleuart);

  // Secondary Scan Response packet (optional)
  // Since there is no room for 'Name' in Advertising packet
  Bluefruit.ScanResponse.addName();
  
  /* Start Advertising
   * - Enable auto advertising if disconnected
   * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
   * - Timeout for fast mode is 30 seconds
   * - Start(timeout) with timeout = 0 will advertise forever (until connected)
   * 
   * For recommended advertising interval
   * https://developer.apple.com/library/content/qa/qa1931/_index.html   
   */
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds  
}

#define TEXT_BUF_LEN 64
char text_buf[TEXT_BUF_LEN];
void loop() {  

  char c = 0;
  bool ble_serial = false;
  if (Serial.available()) {
    Serial.readBytes(&c, 1);
  } else if (bleuart.available()) {
    c = (char) bleuart.read();
    ble_serial = true;
  }

  if (c != 0) {
    if (c == 't')
    {
      if (!currently_tx) 
      {
        currently_tx = true;
        
        // set to transmit
        radio.setModeTransmit();

        Serial.println("Tx");
        int str_len = snprintf(text_buf, TEXT_BUF_LEN, "Tx\n");
        bleuart.write(text_buf, str_len);
        //radio.setTxSourceMic();
        //radio.setRfPower(1);
      } else {
        radio.setModeReceive();
        currently_tx = false;
        Serial.println("Rx");
        int str_len = snprintf(text_buf, TEXT_BUF_LEN, "Rx\n");
        bleuart.write(text_buf, str_len);
      }
    } else if (c == 'F') {
      if (ble_serial == false) {
        Serial.setTimeout(40);
        freq = Serial.parseInt();
        Serial.flush();
      } else {
        int idx = 0;
        while (bleuart.available() && 
          bleuart.peek() >= '0' && 
          bleuart.peek() <= '9' &&
          idx < TEXT_BUF_LEN) {

          text_buf[idx] = bleuart.read();
          idx++;
        }
        text_buf[idx] = 0; // null terminate
        freq = atoi(text_buf);
      }
      radio.frequency(freq);
      Serial.print("set frequency: ");
      Serial.println(freq);
      int str_len = snprintf(text_buf, TEXT_BUF_LEN, "set frequency: %d\n", freq);
      bleuart.write(text_buf, str_len);

    }
  }
  
  if (!currently_tx && (millis() - rssi_timeout) > RSSI_REPORT_RATE_MS)
  {
    int rssi = radio.readRSSI();
    Serial.println(rssi);
    int str_len = snprintf(text_buf, TEXT_BUF_LEN, "rssi: %d\n", rssi);
    bleuart.write(text_buf, str_len);
    rssi_timeout = millis();
  }
}


// callback invoked when central connects
void connect_callback(uint16_t conn_handle)
{
  char central_name[32] = { 0 };
  Bluefruit.Gap.getPeerName(conn_handle, central_name, sizeof(central_name));

  Serial.print("Connected to ");
  Serial.println(central_name);
}

/**
 * Callback invoked when a connection is dropped
 * @param conn_handle connection where this event happens
 * @param reason is a BLE_HCI_STATUS_CODE which can be found in ble_hci.h
 * https://github.com/adafruit/Adafruit_nRF52_Arduino/blob/master/cores/nRF5/nordic/softdevice/s140_nrf52_6.1.1_API/include/ble_hci.h
 */
void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
  (void) conn_handle;
  (void) reason;

  Serial.println();
  Serial.println("Disconnected");
}
