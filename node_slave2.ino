#include <SPI.h>
#include <LoRa.h>
#include "DHT.h"
#include <Wire.h>     // Thêm thư viện I2C
#include <BH1750.h>   // Thêm thư viện cảm biến ánh sáng
#include "esp_task_wdt.h"
//---------------------------------------- 
#define WDT_TIMEOUT 5  // Thời gian timeout 5 giây
#define DHTPIN      14
#define DHTTYPE     DHT11
#define SoilMoisture_Pin  34  // Chân đọc độ ẩm đất
//---------------------------------------- 
#define LED_1_Pin   27
#define LED_2_Pin   26
//---------------------------------------- 
#define LORA_FREQ 433E6 //868E6
#define SCK     18    
#define MISO    19    
#define MOSI    23    
#define SS      5     
#define RST     22    
#define DIO0    2 
//----------------------------------------
DHT dht11(DHTPIN, DHTTYPE);
BH1750 lightMeter;  // Khai báo cảm biến BH1750
//----------------------------------------String variable for LoRa
String Incoming = "";
String Message = "";
String LED_Num = "";
String LED_State = "";
//----------------------------------------
//byte LocalAddress = 0x02;       //--> address of this device (Slave 1).
byte LocalAddress = 0x03;       //--> address of this device (Slave 2).
byte Destination_Master = 0x01; //--> destination to send to Master (ESP32).
//----------------------------------------
int Humd = 0;
int Light = 0;
int SoilMoisture = 0;
float Temp = 0.00;
String send_Status_Read_DHT11 = "";
//---------------------------------------- 

//---------------------------------------- Variable declaration for Millis/Timer.
unsigned long previousMillis_UpdateDHT11 = 0;
const long interval_UpdateDHT11 = 10000;

unsigned long previousMillis_RestartLORA = 0;
const long interval_RestartLORA = 1000;
//---------------------------------------- 

// Declaration of variable as counter to restart Lora Ra-02.
byte Count_to_Rst_LORA = 0;
int retryCount = 0;
//________________________________________________________________________________ Subroutines for sending data (LoRa Ra-02).
void sendMessage(String Outgoing, byte Destination) {
  LoRa.beginPacket();             //--> start packet
  LoRa.write(Destination);        //--> add destination address
  LoRa.write(LocalAddress);       //--> add sender address
  LoRa.write(Outgoing.length());  //--> add payload length
  LoRa.print(Outgoing);           //--> add payload
  LoRa.endPacket();               //--> finish packet and send it
  LoRa.receive();
}
//________________________________________________________________________________ 

//________________________________________________________________________________ Subroutines for receiving data (LoRa Ra-02).
void onReceive(int packetSize) {
  if (packetSize == 0) return;          // if there's no packet, return

  //---------------------------------------- read packet header bytes:
  int recipient = LoRa.read();        //--> recipient address
  byte sender = LoRa.read();          //--> sender address
  byte incomingLength = LoRa.read();  //--> incoming msg length
  byte master_Send_Mode = LoRa.read();
  //---------------------------------------- 

  //---------------------------------------- Condition that is executed if message is not from Master.
  if (sender != Destination_Master) {
    Serial.println();
    Serial.println("Not from Master, Ignore");
    Serial.println(sender, HEX);
    // Resets the value of the Count_to_Rst_LORA variable.
    Count_to_Rst_LORA = 0;
    return; //--> skip rest of function
  }
  // Clears Incoming variable data.
  Incoming = "";
  //---------------------------------------- Get all incoming data.
  while (LoRa.available()) {
    Incoming += (char)LoRa.read();
  }
  //---------------------------------------- 

  // Resets the value of the Count_to_Rst_LORA variable.
  Count_to_Rst_LORA = 0;

  //---------------------------------------- Check length for error.
  if (incomingLength != Incoming.length()) {
    Serial.println("error: message length does not match length");
    while (LoRa.available()) LoRa.read();
    return; //--> skip rest of function
  }
  //---------------------------------------- Checks whether the incoming data or message for this device.
  if (recipient != LocalAddress) {
    Serial.println("This message is not for me.");
    //Processing_incoming_data_for_LEDs();
    return; //--> skip rest of function
  } else {
    // if message is for this device, or broadcast, print details:
    Serial.println();
    Serial.println("Rc from: 0x" + String(sender, HEX));
    Serial.println("Message: " + Incoming);

    // Calls the Processing_incoming_data() subroutine.
    if (master_Send_Mode == 1) Processing_incoming_data();
    
    // Calls the Processing_incoming_data_for_Ctrl_LEDs() subroutine.
    if (master_Send_Mode == 2) Processing_incoming_data_for_Ctrl_LEDs();
  }
}
//________________________________________________________________________________ 

//________________________________________________________________________________ Subroutine to process the data to be sent, after that it sends a message to the Master.
void Processing_incoming_data() {
    byte LED_1_State = digitalRead(LED_1_Pin);
    byte LED_2_State = digitalRead(LED_2_Pin);

    // Gửi dữ liệu đầy đủ
    Message = send_Status_Read_DHT11 + "," + 
              String(Humd) + "," + 
              String(Temp) + "," + 
              String(Light) + "," + 
              String(SoilMoisture) + "," + 
              String(LED_1_State) + "," + 
              String(LED_2_State);

    Serial.println();
    Serial.println("Tr to  : 0x" + String(Destination_Master, HEX));
    Serial.println("Message: " + Message);

    sendMessage(Message, Destination_Master);
}
//________________________________________________________________________________ 

//________________________________________________________________________________ Subroutines for processing data from incoming messages to Controlling the LEDs.
void Processing_incoming_data_for_Ctrl_LEDs() {

  LED_Num = GetValue(Incoming, ',', 0);
  LED_State = GetValue(Incoming, ',', 1);

  if (LED_Num == "1") {
    if (LED_State == "t") digitalWrite(LED_1_Pin, HIGH);
    if (LED_State == "f") digitalWrite(LED_1_Pin, LOW);
  }

  if (LED_Num == "2") {
    if (LED_State == "t") digitalWrite(LED_2_Pin, HIGH);
    if (LED_State == "f") digitalWrite(LED_2_Pin, LOW);
  }
}
//________________________________________________________________________________ 
String GetValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;
  
  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  if (found <= index) return ""; 
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
//________________________________________________________________________________ Subroutine to reset Lora Ra-02.
void Rst_LORA() {
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DIO0);

  Serial.println();
  Serial.println("Restart LoRa...");
  while (!LoRa.begin(LORA_FREQ) && retryCount < 5) {             // initialize ratio at 915 or 433 MHz
    Serial.println("LoRa init failed. Retrying...");
    delay(1000);
    retryCount++;
  }
  if (retryCount >= 5) {
    Serial.println("[FATAL] LoRa init failed permanently. Restarting ESP32...");
    ESP.restart(); // Reset ESP32 nếu LoRa không hoạt động
  } else {
    Serial.println("LoRa init succeeded.");
  }
  // Reset the value of the Count_to_Rst_LORA variable.
  Count_to_Rst_LORA = 0;
  retryCount = 0;
}
//________________________________________________________________________________ VOID SETUP
void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);

  pinMode(LED_1_Pin, OUTPUT);
  pinMode(LED_2_Pin, OUTPUT);

// Cấu hình và khởi tạo watchdog timer (Cú pháp v2.x)
  esp_task_wdt_init(WDT_TIMEOUT, true); 
  esp_task_wdt_add(NULL); // Đăng ký watchdog cho loop() chính

  // Calls the Rst_LORA() subroutine.
  Rst_LORA();
  // pinMode(SoilMoisture_Pin, INPUT);
  // Cấu hình I2C cho BH1750 (SDA = D21, SCL = D15)
  dht11.begin();
  Wire.begin(32, 33);   
  lightMeter.begin();
  Incoming.reserve(100);
  Message.reserve(100);
}
//________________________________________________________________________________ 

//________________________________________________________________________________ VOID LOOP
void loop() {
  esp_task_wdt_reset();

  //---------------------------------------- Millis / Timer to update the temperature and humidity values ​​from the DHT11 sensor every 2 seconds (see the variable interval_UpdateDHT11).
  unsigned long currentMillis_UpdateDHT11 = millis();
  
  if (currentMillis_UpdateDHT11 - previousMillis_UpdateDHT11 >= interval_UpdateDHT11) {
    previousMillis_UpdateDHT11 = currentMillis_UpdateDHT11;

    Humd = dht11.readHumidity();
    Temp = dht11.readTemperature();

    if (isnan(Humd) || isnan(Temp)) {
      Humd = 0;
      Temp = 0.00;
      send_Status_Read_DHT11 = "f";
      Serial.println(F("Failed to read from DHT sensor!"));
      //return;
    } else {
      send_Status_Read_DHT11 = "s";
    }
    Light = lightMeter.readLightLevel();
    // Đọc giá trị độ ẩm đất (quy đổi về phần trăm)
    SoilMoisture = map(analogRead(SoilMoisture_Pin), 0, 4095, 100, 0);
  }

  unsigned long currentMillis_RestartLORA = millis();
  
  if (currentMillis_RestartLORA - previousMillis_RestartLORA >= interval_RestartLORA) {
    previousMillis_RestartLORA = currentMillis_RestartLORA;

    Count_to_Rst_LORA++;
    if (Count_to_Rst_LORA > 30) {
      LoRa.end();
      Rst_LORA();
    }
  }
  //---------------------------------------- parse for a packet, and call onReceive with the result:
  onReceive(LoRa.parsePacket());
}