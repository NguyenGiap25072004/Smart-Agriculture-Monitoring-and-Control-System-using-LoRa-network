#include <SPI.h>
#include <LoRa.h>
#include <AsyncTCP.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <FirebaseESP32.h>
#include <ESPAsyncWebServer.h>
#include <Arduino_JSON.h>
#include <esp_task_wdt.h>

#include "web_dashboard.h" 

// cau hinh firebase
#define FIREBASE_AUTH "xIFVYT1WLjLKTJX6sztlo9SLyJUmKI4Wr4bkJKau"

#define DATABASE_URL "https://smartfarmvju-default-rtdb.asia-southeast1.firebasedatabase.app"  // URL của Firebase

FirebaseData firebaseData;
FirebaseAuth auth;
FirebaseConfig config;


// Khai báo NTP Client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7 * 3600, 60000);  // Múi giờ GMT+7 (Việt Nam)
//---------------------------------------- LoRa Pin / GPIO configuration.
#define WDT_TIMEOUT 5  // Thời gian watchdog timeout (5 giây)

// #define Button_1_Pin  13 //--> D13
// #define Button_2_Pin  12 //--> D12
// #define Button_3_Pin  14 //--> D14
// #define Button_4_Pin  27 //--> D27
#define LORA_FREQ 433E6 //868E6
//---------------------------------------- LoRa Pin / GPIO configuration.
#define SCK     18    
#define MISO    19    
#define MOSI    23    
#define SS      5     
#define RST     22    
#define DIO0    2   
//----------------------------------------

//---------------------------------------- Variable declaration for your network credentials.
const char* ssid = "Truong Giang Nguyen";
const char* password = "hoilamgi";
//----------------------------------------

//---------------------------------------- Variable declaration to hold incoming and outgoing data.
String Incoming = "";
String Message = ""; 
//----------------------------------------

//---------------------------------------- LoRa data transmission configuration.
byte LocalAddress = 0x01;               //--> address of this device (Master Address).
byte Destination_ESP32_Slave_1 = 0x02;  //--> destination to send to Slave 1 (ESP32).
byte Destination_ESP32_Slave_2 = 0x03;  //--> destination to send to Slave 2 (ESP32).
const byte get_Data_Mode = 1;           //--> Mode to get the reading status of the DHT11 sensor, humidity value, temperature value, state of LED 1 and LED 2.
const byte led_Control_Mode = 2;        //--> Mode to control LED 1 and LED 2 on the targeted Slave.
//---------------------------------------- 

//---------------------------------------- Variable declaration for Millis/Timer.
unsigned long previousMillis_SendMSG_to_GetData = 0;
const long interval_SendMSG_to_GetData = 10000;

unsigned long previousMillis_RestartLORA = 0;
const long interval_RestartLORA = 5000;
//---------------------------------------- Variables to accommodate the reading status of the DHT11 sensor, humidity value, temperature value, state of LED 1 and LED 2.
int Humd[2], Light[2], SoilMoisture[2];
float Temp[2] ;
String LED_1_State_str = "";
String LED_2_State_str = "";
String receive_Status_Read_DHT11 = "";
bool LED_1_State_bool = false;
bool LED_2_State_bool = false;

// 2 biến lưu RSSI và khoảng cách
int current_rssi = 0;
float current_distance = 0.0;
String ai_decision_S1 = "SAFE";
String ai_decision_S2 = "SAFE";

//---------------------------------------- 
const char* PARAM_INPUT_1 = "Slave_Num";
const char* PARAM_INPUT_2 = "LED_Num";
const char* PARAM_INPUT_3 = "LED_Val";
//---------------------------------------- Variable declaration to hold data from the web server to control the LED.
String Slave_Number = "";
String LED_Number = "";
String LED_Value = "";
// Variable declaration to count slaves.
byte Slv = 0;
// Variable declaration to get the address of the slaves.
byte slave_Address;
// Declaration of variable as counter to restart Lora Ra-02.
byte count_to_Rst_LORA = 0;
// Variable declaration to notify that the process of receiving the message has finished.
bool finished_Receiving_Message = false;

// Variable declaration to notify that the process of sending the message has finished.
bool finished_Sending_Message = false;

// Variable declaration to start sending messages to Slaves to control the LEDs.
bool send_Control_LED = false;

// Initialize JSONVar
JSONVar JSON_All_Data_Received;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create an Event Source on /events
AsyncEventSource events("/events");

//________________________________________________________________________________ Subroutines for sending data (LoRa Ra-02).
void sendMessage(String Outgoing, byte Destination, byte SendMode) { 
  finished_Sending_Message = false;

  Serial.println();
  Serial.println("Tr to  : 0x" + String(Destination, HEX));
  Serial.print("Mode   : ");
  if (SendMode == 1) Serial.println("Getting Data");
  if (SendMode == 2) Serial.println("Controlling LED.");
  Serial.println("Message: " + Outgoing);
  
  LoRa.beginPacket();             //--> start packet
  LoRa.write(Destination);        //--> add destination address
  LoRa.write(LocalAddress);       //--> add sender address
  LoRa.write(Outgoing.length());  //--> add payload length
  LoRa.write(SendMode);
  LoRa.print(Outgoing);           //--> add payload
  LoRa.endPacket();               //--> finish packet and send it
  LoRa.receive();
  finished_Sending_Message = true;
}
//________________________________________________________________________________ 

//________________________________________________________________________________ Subroutines for receiving data (LoRa Ra-02).
void onReceive(int packetSize) {
  if (packetSize == 0) return;          // if there's no packet, return
  finished_Receiving_Message = false;
  //---------------------------------------- read packet header bytes:
  int recipient = LoRa.read();        //--> recipient address
  byte sender = LoRa.read();          //--> sender address
  byte incomingLength = LoRa.read();  //--> incoming msg length
  // Clears Incoming variable data.
  Incoming = "";
//---------------------------------------- Get all incoming data / message.
  while (LoRa.available()) {
    Incoming += (char)LoRa.read();
  }
  
  // Lấy chỉ số RSSI và tính khoảng cách
  current_rssi = LoRa.packetRssi();
  float A = -45.0; // RSSI tiêu chuẩn ở 1 mét (có thể tinh chỉnh sau)
  float n = 3.0;   // Hệ số suy hao môi trường
  current_distance = pow(10, (A - current_rssi) / (10.0 * n));

  // Resets the value of the count_to_Rst_LORA variable if a message is received.
  count_to_Rst_LORA = 0;

  //---------------------------------------- Check length for error.
  if (incomingLength != Incoming.length()) {
    Serial.println("error: message length does not match length");
    finished_Receiving_Message = true;
    return; //--> skip rest of function
  }
  //---------------------------------------- Checks whether the incoming data or message for this device.
  if (recipient != LocalAddress) {
    Serial.println("This message is not for me.");
    finished_Receiving_Message = true;
    return; //--> skip rest of function
  }
  //----------------------------------------  if message is for this device, or broadcast, print details:
  Serial.println();
  Serial.println("Rc from: 0x" + String(sender, HEX));
  Serial.println("Message: " + Incoming);
  //---------------------------------------- 

  // Get the address of the senders or slaves.
  slave_Address = sender;

  // Calls the Processing_incoming_data() subroutine.
  Processing_incoming_data();

  finished_Receiving_Message = true;
}
//________________________________________________________________________________ 

void Send_Data_to_Firebase(String node, int index) {
    String path = "/" + node;  // Lưu dữ liệu vào /Node1/
    String idPath = path + "/latestID";  // Lưu ID mới nhất

    // Lấy ID hiện tại từ Firebase
    int latestID = 0;
    if (Firebase.getInt(firebaseData, idPath)) {  
        latestID = firebaseData.intData();
    } 

    int newID = latestID + 1;  // Tạo ID mới tăng dần
    String newIDStr = String(newID);  // Chuyển thành chuỗi

    // Cập nhật thời gian từ NTP Server
    timeClient.update();
    String currentTime = getFormattedTime();

    FirebaseJson json;
    json.set("Humidity", Humd[index]);
    json.set("Temperature", Temp[index]);
    json.set("Light", Light[index]);
    json.set("SoilMoisture", SoilMoisture[index]);
    json.set("LED_1", LED_1_State_str);  // Trạng thái LED 1
    json.set("LED_2", LED_2_State_str);  // Trạng thái LED 2
    json.set("timestamp", currentTime);

    // Gửi dữ liệu lên Firebase với ID mới
    String dataPath = path + "/" + newIDStr;
    if (Firebase.setJSON(firebaseData, dataPath, json)) {
        Serial.println("Dữ liệu đã gửi lên Firebase thành công!");
        Serial.println("ID mới: " + newIDStr);

        // Cập nhật latestID trong Firebase
        Firebase.setInt(firebaseData, idPath, newID);
    } else {
        Serial.print("Lỗi Firebase: ");
        Serial.println(firebaseData.errorReason());
    }
}



// Hàm lấy thời gian thực từ NTP
String getFormattedTime() {
    time_t rawTime = timeClient.getEpochTime();
    struct tm *timeInfo = localtime(&rawTime);
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", timeInfo);
    return String(buffer);
}


//________________________________________________________________________________ Subroutines to process data from incoming messages.
void Processing_incoming_data() {
  
  if (slave_Address == Destination_ESP32_Slave_1) {
    receive_Status_Read_DHT11 = GetValue(Incoming, ',', 0);
    if (receive_Status_Read_DHT11 == "f") receive_Status_Read_DHT11 = "FAILED";
    if (receive_Status_Read_DHT11 == "s") receive_Status_Read_DHT11 = "SUCCEED";
    Humd[0] = GetValue(Incoming, ',', 1).toInt();
    Temp[0] = GetValue(Incoming, ',', 2).toFloat();
    Light[0] = GetValue(Incoming, ',', 3).toInt();  // Thêm đọc giá trị ánh sáng
    SoilMoisture[0] = GetValue(Incoming, ',', 4).toInt();  // Thêm đọc độ ẩm đất
    LED_1_State_str = GetValue(Incoming, ',', 5);
    LED_2_State_str = GetValue(Incoming, ',', 6);
    if (LED_1_State_str == "1" || LED_1_State_str == "0") {
      LED_1_State_bool = LED_1_State_str.toInt();
    }
    if (LED_2_State_str == "1" || LED_2_State_str == "0") {
      LED_2_State_bool = LED_2_State_str.toInt();
    }

    // Calls the Send_Data_to_WS() subroutine.
    Send_Data_to_WS("S1", 1);
    Send_Data_to_Firebase("Node1", 0);

    // --- Sent rssi & distance node 1 to firebase ---
    Firebase.setInt(firebaseData, "/Network_Config/Active_Nodes/Node1/rssi", current_rssi);
    Firebase.setFloat(firebaseData, "/Network_Config/Active_Nodes/Node1/distance", current_distance);

    Serial.println("Node1: Humidity: " + String(Humd[0]) + 
               ", Temp: " + String(Temp[0]) + 
               ", Lux: " + String(Light[0]) + 
               ", Soil Moisture: " + String(SoilMoisture[0]) + "% , " + 
               "LED 1: " + String(LED_1_State_str) + 
               ", LED 2: " + String(LED_2_State_str));
    
    // =================================================================
    // KỊCH BẢN ĐIỀU KHIỂN THÔNG MINH 3 CHẾ ĐỘ CHO NODE 1
    // =================================================================
    String ctrlMode_S1 = "MANUAL"; // Mặc định nếu không đọc được
    if (Firebase.getString(firebaseData, "/Network_Config/Active_Nodes/Node1/control_mode")) {
        ctrlMode_S1 = firebaseData.stringData();
    }

    // --- CHẾ ĐỘ 1: ĐIỀU KHIỂN BẰNG AI ---
    if (ctrlMode_S1 == "AI") {
        String aiDecision = "SAFE";
        if (Firebase.getString(firebaseData, "/Network_Config/Active_Nodes/Node1/ai_decision")) {
            aiDecision = firebaseData.stringData();
        }
        
        // Chỉ bật bơm (lệnh "2,t") nếu AI yêu cầu PUMP_ON và trạng thái bơm hiện tại đang tắt ("0")
        if (aiDecision == "PUMP_ON" && LED_2_State_str == "0") {
            Serial.println("[AI MODE] AI yêu cầu PUMP_ON. Tiến hành bật bơm Node 1...");
            delay(100);
            sendMessage("2,t", Destination_ESP32_Slave_1, led_Control_Mode);
        } 
        // Chỉ tắt bơm (lệnh "2,f") nếu AI báo SAFE và trạng thái bơm hiện tại đang bật ("1")
        else if (aiDecision == "SAFE" && LED_2_State_str == "1") {
            Serial.println("[AI MODE] AI báo SAFE. Tiến hành tắt bơm Node 1...");
            delay(100);
            sendMessage("2,f", Destination_ESP32_Slave_1, led_Control_Mode);
        }
    }
    // --- CHẾ ĐỘ 2: ĐIỀU KHIỂN TỰ ĐỘNG THÔNG THƯỜNG (BIẾN THƯỜNG) ---
    else if (ctrlMode_S1 == "AUTO") {
        // Tự động bật bơm nếu độ ẩm đất < 55% và bơm đang tắt
        if (SoilMoisture[0] > 0 && SoilMoisture[0] < 55 && LED_2_State_str == "0") {
            Serial.println("[AUTO MODE] Độ ẩm đất < 55%. Tiến hành bật bơm Node 1...");
            delay(100);
            sendMessage("2,t", Destination_ESP32_Slave_1, led_Control_Mode);
        }
        // Tự động tắt bơm nếu độ ẩm đất đạt ngưỡng an toàn >= 70% và bơm đang bật
        else if (SoilMoisture[0] >= 70 && LED_2_State_str == "1") {
            Serial.println("[AUTO MODE] Đất đã đủ ẩm (>= 70%). Tiến hành tắt bơm Node 1...");
            delay(100);
            sendMessage("2,f", Destination_ESP32_Slave_1, led_Control_Mode);
        }

        // Tự động điều khiển Quạt tản nhiệt (LED 1) theo nhiệt độ không khí
        if (Temp[0] > 32.0 && LED_1_State_str == "0") {
            Serial.println("[AUTO MODE] Nhiệt độ > 32°C. Tiến hành bật quạt Node 1...");
            delay(100);
            sendMessage("1,t", Destination_ESP32_Slave_1, led_Control_Mode);
        }
        else if (Temp[0] < 29.0 && Temp[0] > 0 && LED_1_State_str == "1") {
            Serial.println("[AUTO MODE] Nhiệt độ < 29°C. Tiến hành tắt quạt Node 1...");
            delay(100);
            sendMessage("1,f", Destination_ESP32_Slave_1, led_Control_Mode);
        }
    }
    // --- CHẾ ĐỘ 3: MANUAL ---
    // Nếu ctrlMode là "MANUAL", Master giữ nguyên trạng thái để người dùng tự bật/tắt bằng tay từ Web
  }
  //---------------------------------------- 

  //---------------------------------------- Conditions for processing data or messages from Slave 2 (ESP32 Slave 2).
  if (slave_Address == Destination_ESP32_Slave_2) {
    receive_Status_Read_DHT11 = GetValue(Incoming, ',', 0);
    if (receive_Status_Read_DHT11 == "f") receive_Status_Read_DHT11 = "FAILED";
    if (receive_Status_Read_DHT11 == "s") receive_Status_Read_DHT11 = "SUCCEED";
    Humd[1] = GetValue(Incoming, ',', 1).toInt();
    Temp[1] = GetValue(Incoming, ',', 2).toFloat();
    Light[1] = GetValue(Incoming, ',', 3).toInt();  // Thêm đọc giá trị ánh sáng
    SoilMoisture[1] = GetValue(Incoming, ',', 4).toInt();  // Thêm đọc độ ẩm đất
    LED_1_State_str = GetValue(Incoming, ',', 5);
    LED_2_State_str = GetValue(Incoming, ',', 6);
    if (LED_1_State_str == "1" || LED_1_State_str == "0") {
      LED_1_State_bool = LED_1_State_str.toInt();
    }
    if (LED_2_State_str == "1" || LED_2_State_str == "0") {
      LED_2_State_bool = LED_2_State_str.toInt();
    }

    // Calls the Send_Data_to_WS() subroutine.
    Send_Data_to_WS("S2", 2);
    Send_Data_to_Firebase("Node2", 1);

    // --- Sent rssi & distance node 2 to firebase ---
    Firebase.setInt(firebaseData, "/Network_Config/Active_Nodes/Node2/rssi", current_rssi);
    Firebase.setFloat(firebaseData, "/Network_Config/Active_Nodes/Node2/distance", current_distance);

    Serial.println("Node2: Humidity: " + String(Humd[1]) + 
               ", Temp: " + String(Temp[1]) + 
               ", Lux: " + String(Light[1]) + 
               ", Soil Moisture: " + String(SoilMoisture[1]) + "% , " + 
               "LED 1: " + String(LED_1_State_str) + 
               ", LED 2: " + String(LED_2_State_str));
    
    // =================================================================
    // KỊCH BẢN ĐIỀU KHIỂN THÔNG MINH 3 CHẾ ĐỘ CHO NODE 2
    // =================================================================
    String ctrlMode_S2 = "MANUAL";
    if (Firebase.getString(firebaseData, "/Network_Config/Active_Nodes/Node2/control_mode")) {
        ctrlMode_S2 = firebaseData.stringData();
    }

    // --- CHẾ ĐỘ 1: ĐIỀU KHIỂN BẰNG AI ---
    if (ctrlMode_S2 == "AI") {
        String aiDecision = "SAFE";
        if (Firebase.getString(firebaseData, "/Network_Config/Active_Nodes/Node2/ai_decision")) {
            aiDecision = firebaseData.stringData();
        }
        
        if (aiDecision == "PUMP_ON" && LED_2_State_str == "0") {
            Serial.println("[AI MODE] AI yêu cầu PUMP_ON. Tiến hành bật bơm Node 2...");
            delay(100);
            sendMessage("2,t", Destination_ESP32_Slave_2, led_Control_Mode);
        } 
        else if (aiDecision == "SAFE" && LED_2_State_str == "1") {
            Serial.println("[AI MODE] AI báo SAFE. Tiến hành tắt bơm Node 2...");
            delay(100);
            sendMessage("2,f", Destination_ESP32_Slave_2, led_Control_Mode);
        }
    }
    // --- CHẾ ĐỘ 2: ĐIỀU KHIỂN TỰ ĐỘNG THÔNG THƯỜNG ---
    else if (ctrlMode_S2 == "AUTO") {
        if (SoilMoisture[1] > 0 && SoilMoisture[1] < 55 && LED_2_State_str == "0") {
            Serial.println("[AUTO MODE] Độ ẩm đất < 55%. Tiến hành bật bơm Node 2...");
            delay(100);
            sendMessage("2,t", Destination_ESP32_Slave_2, led_Control_Mode);
        }
        else if (SoilMoisture[1] >= 70 && LED_2_State_str == "1") {
            Serial.println("[AUTO MODE] Đất đã đủ ẩm (>= 70%). Tiến hành tắt bơm Node 2...");
            delay(100);
            sendMessage("2,f", Destination_ESP32_Slave_2, led_Control_Mode);
        }

        if (Temp[1] > 32.0 && LED_1_State_str == "0") {
            Serial.println("[AUTO MODE] Nhiệt độ > 32°C. Tiến hành bật quạt Node 2...");
            delay(100);
            sendMessage("1,t", Destination_ESP32_Slave_2, led_Control_Mode);
        }
        else if (Temp[1] < 29.0 && Temp[1] > 0 && LED_1_State_str == "1") {
            Serial.println("[AUTO MODE] Nhiệt độ < 29°C. Tiến hành tắt quạt Node 2...");
            delay(100);
            sendMessage("1,f", Destination_ESP32_Slave_2, led_Control_Mode);
        }
    }

  }
  //---------------------------------------- 
}
//________________________________________________________________________________ 

//________________________________________________________________________________ Subroutine to send data received from Slaves to the web server to be displayed.
void Send_Data_to_WS(char ID_Slave[5], byte Slave) {
  //:::::::::::::::::: Enter the received data into JSONVar(JSON_All_Data_Received).
  JSON_All_Data_Received["ID_Slave"] = ID_Slave;
  JSON_All_Data_Received["StatusReadDHT11"] = receive_Status_Read_DHT11;
  JSON_All_Data_Received["Humd"] = Humd[Slave-1];
  JSON_All_Data_Received["Temp"] = Temp[Slave-1];
  JSON_All_Data_Received["Light"] = Light[Slave-1];
  JSON_All_Data_Received["SoilMoisture"] = SoilMoisture[Slave-1];
  JSON_All_Data_Received["LED1"] = LED_1_State_bool;
  JSON_All_Data_Received["LED2"] = LED_2_State_bool; 
  // --- GỬI KHOẢNG CÁCH XUỐNG WEB ---
  JSON_All_Data_Received["RSSI"] = current_rssi;
  JSON_All_Data_Received["Distance"] = current_distance;
  //:::::::::::::::::: 
  
  // --- ĐÓNG GÓI TRẠNG THÁI AI GỬI XUỐNG WEB ---
  if (Slave == 1) {
    JSON_All_Data_Received["AIDecision"] = ai_decision_S1;
  } else if (Slave == 2) {
    JSON_All_Data_Received["AIDecision"] = ai_decision_S2;
  }
  // ------------------------------------------------------------

  //:::::::::::::::::: Create a JSON String to hold all data received from the sender.
  String jsonString_Send_All_Data_received = JSON.stringify(JSON_All_Data_Received);
  //:::::::::::::::::: 
  
  //:::::::::::::::::: Sends all data received from the sender to the browser as an event ('allDataJSON').
  events.send(jsonString_Send_All_Data_received.c_str(), "allDataJSON", millis());
  //::::::::::::::::::  
}
//________________________________________________________________________________ 

//________________________________________________________________________________ String function to process the data received
// I got this from : https://www.electroniclinic.com/reyax-lora-based-multiple-sensors-monitoring-using-arduino/
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
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
//________________________________________________________________________________ 

void Rst_LORA() {
  int retryCount = 0;
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
  count_to_Rst_LORA = 0;
}
//________________________________________________________________________________ VOID SETUP
void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);

  //---------------------------------------- Clears the values of the Temp and Humd array variables for the first time.
  for (byte i = 0; i < 2; i++) {
    Humd[i] = 0;
    Temp[i] = 0.00;
    Light[i] = 0;
    SoilMoisture[i] = 0;
  }
  //---------------------------------------- 

  //---------------------------------------- Set Wifi to STA mode
  Serial.println();
  Serial.println("-------------");
  Serial.println("WIFI mode : STA");
  WiFi.mode(WIFI_STA);
  Serial.println("-------------");
  //---------------------------------------- 

  delay(100);

  //---------------------------------------- Connect to Wi-Fi (STA).
  Serial.println("------------");
  Serial.println("WIFI STA");
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  // Cấu hình và khởi tạo watchdog timer (Cú pháp v2.x)
  esp_task_wdt_init(WDT_TIMEOUT, true); 
  esp_task_wdt_add(NULL); // Đăng ký watchdog cho loop() chính
    

  int connecting_process_timed_out = 20; //--> 20 = 20 seconds.
  connecting_process_timed_out = connecting_process_timed_out * 2;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    if(connecting_process_timed_out > 0) connecting_process_timed_out--;
    if(connecting_process_timed_out == 0) {
      delay(1000);
      ESP.restart();
    }
  }
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("------------");
  //:::::::::::::::::: 
  //---------------------------------------- 

  delay(500);

  //---------------------------------------- Handle Web Server
  Serial.println();
  Serial.println("Setting Up the Main Page on the Server.");
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", MAIN_page);
  });
  //---------------------------------------- 

  //---------------------------------------- Handle Web Server Events
  Serial.println();
  Serial.println("Setting up event sources on the Server.");
  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 10 second
    client->send("hello!", NULL, millis(), 10000);
  });
  //---------------------------------------- 

  //---------------------------------------- Send a GET request to <ESP_IP>/set_LED?Slave_Num=<inputMessage1>&LED_Num=<inputMessage2>&LED_Val=<inputMessage3>
  server.on("/set_LED", HTTP_GET, [] (AsyncWebServerRequest *request) {
    //:::::::::::::::::: 
    // GET input value on <ESP_IP>/set_LED?Slave_Num=<inputMessage1>&LED_Num=<inputMessage2>&LED_Val=<inputMessage3>
    // PARAM_INPUT_1 = inputMessage1
    // PARAM_INPUT_2 = inputMessage2
    // PARAM_INPUT_3 = inputMessage3
    // Slave_Number = PARAM_INPUT_1
    // LED_Number = PARAM_INPUT_2
    // LED_Value = PARAM_INPUT_3
    //:::::::::::::::::: 
    
    if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2) && request->hasParam(PARAM_INPUT_3)) {
      Slave_Number = request->getParam(PARAM_INPUT_1)->value();
      LED_Number = request->getParam(PARAM_INPUT_2)->value();
      LED_Value = request->getParam(PARAM_INPUT_3)->value();

      String Rslt = "Slave : " + Slave_Number + " || LED : " + LED_Number + " || Set to : " + LED_Value;
      Serial.println();
      Serial.println(Rslt);
      send_Control_LED = true;
    }
    else {
      send_Control_LED = false;
      Slave_Number = "No message sent";
      LED_Number = "No message sent";
      LED_Value = "No message sent";
      String Rslt = "Slave : " + Slave_Number + " || LED : " + LED_Number + " || Set to : " + LED_Value;
      Serial.println();
      Serial.println(Rslt);
    }
    request->send(200, "text/plain", "OK");
  });
  //---------------------------------------- 

  //---------------------------------------- 
  // API Lắng nghe lệnh Xóa Node từ Web Dashboard
  server.on("/delete_node", HTTP_GET, [] (AsyncWebServerRequest *request) {
    if (request->hasParam("node")) {
      String targetNode = request->getParam("node")->value(); // Lấy tên Node cần xóa (VD: "Node1")
      
      // Ghi đè trạng thái is_active thành false thẳng lên Firebase
      String path = "/Network_Config/Active_Nodes/" + targetNode + "/is_active";
      Firebase.setBool(firebaseData, path, false);
      
      Serial.println();
      Serial.println("[HỆ THỐNG WEB] Đã nhận lệnh khóa/xóa: " + targetNode);
    }
    request->send(200, "text/plain", "Node Deleted");
  });
  //----------------------------------------

  //---------------------------------------- 
  // API Lắng nghe lệnh Khám phá mạng (Auto-Discovery) từ Web
  server.on("/scan_network", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String foundNodes = "["; // Tạo chuỗi JSON mảng
    bool isFirst = true;
    
    Serial.println("[HỆ THỐNG] Đang quét các Node mới/bị khóa trong phạm vi...");

    // Thử "Ping" Node 1 (Nếu nó đang bị khóa trên Firebase)
    if (Firebase.getBool(firebaseData, "/Network_Config/Active_Nodes/Node1/is_active") && !firebaseData.boolData()) {
      sendMessage("", Destination_ESP32_Slave_1, get_Data_Mode); // Gọi Node 1
      unsigned long startWait = millis();
      bool isAlive = false;
      while(millis() - startWait < 1500) { // Chờ 1.5 giây xem có phản hồi không
        if (LoRa.parsePacket()) { isAlive = true; break; }
      }
      if (isAlive) {
        foundNodes += "\"Node1\"";
        isFirst = false;
        Serial.println("-> Đã tìm thấy: Node 1");
      }
    }

    // Thử "Ping" Node 2 (Nếu nó đang bị khóa trên Firebase)
    if (Firebase.getBool(firebaseData, "/Network_Config/Active_Nodes/Node2/is_active") && !firebaseData.boolData()) {
      sendMessage("", Destination_ESP32_Slave_2, get_Data_Mode); // Gọi Node 2
      unsigned long startWait = millis();
      bool isAlive = false;
      while(millis() - startWait < 1500) { 
        if (LoRa.parsePacket()) { isAlive = true; break; }
      }
      if (isAlive) {
        if (!isFirst) foundNodes += ",";
        foundNodes += "\"Node2\"";
        Serial.println("-> Đã tìm thấy: Node 2");
      }
    }

    foundNodes += "]"; // Đóng mảng JSON
    request->send(200, "application/json", foundNodes); // Trả kết quả về cho Web
  });

  // API Thêm Node (Kết nối lại)
  server.on("/add_node", HTTP_GET, [] (AsyncWebServerRequest *request) {
    if (request->hasParam("node")) {
      String targetNode = request->getParam("node")->value();
      
      // Bật lại is_active = true trên Firebase
      String path = "/Network_Config/Active_Nodes/" + targetNode + "/is_active";
      Firebase.setBool(firebaseData, path, true);
      
      Serial.println("[HỆ THỐNG] Đã kết nối lại thành công với: " + targetNode);
    }
    request->send(200, "text/plain", "Node Added");
  });

//---------------------------------------- 
  // API Lắng nghe lệnh Đổi Chế độ (MANUAL / AUTO / AI) từ Web Dashboard
  server.on("/set_mode", HTTP_GET, [] (AsyncWebServerRequest *request) {
    if (request->hasParam("node") && request->hasParam("mode")) {
      String targetNode = request->getParam("node")->value(); // Lấy tên Node (VD: "Node1")
      String newMode = request->getParam("mode")->value();    // Lấy chế độ (VD: "AI")
      
      // Ghi đè trạng thái điều khiển lên Firebase
      String path = "/Network_Config/Active_Nodes/" + targetNode + "/control_mode";
      Firebase.setString(firebaseData, path, newMode);
      
      Serial.println();
      Serial.println("[HỆ THỐNG WEB] Đã đổi " + targetNode + " sang chế độ: " + newMode);
    }
    request->send(200, "text/plain", "Mode Updated");
  });
  //----------------------------------------

  //----------------------------------------

  //---------------------------------------- Adding event sources on the Server.
  Serial.println();
  Serial.println("Adding event sources on the Server.");
  server.addHandler(&events);
  //---------------------------------------- 

  //---------------------------------------- Starting the Server.
  Serial.println();
  Serial.println("Starting the Server.");
  server.begin();
  //---------------------------------------- 

  //cau hinh fire base
// Cấu hình Firebase
 // Cấu hình Firebase
  config.database_url = DATABASE_URL;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("Firebase kết nối thành công!");

  ///
   // Khởi động NTP
    timeClient.begin();
    timeClient.update();

  // Calls the Rst_LORA() subroutine.
  Rst_LORA();
  Incoming.reserve(100);
  Message.reserve(100);

  Serial.println();
  Serial.println("------------");
  Serial.print("ESP32 IP address : ");
  Serial.println(WiFi.localIP());
  Serial.println();
  Serial.println("Visit the IP Address above in your browser to open the main page.");
  Serial.println("------------");
  Serial.println();
}
//________________________________________________________________________________ 

//________________________________________________________________________________ VOID LOOP
void loop() {
  esp_task_wdt_reset();
  unsigned long currentMillis_SendMSG_to_GetData = millis();
  
  if (currentMillis_SendMSG_to_GetData - previousMillis_SendMSG_to_GetData >= interval_SendMSG_to_GetData) {
    previousMillis_SendMSG_to_GetData = currentMillis_SendMSG_to_GetData;

    Slv++;
    if (Slv > 2) Slv = 1;
    
    //:::::::::::::::::: Cập nhật Logic Quản lý Node 1
    if (Slv == 1) {
      bool isActive = true;
      if (Firebase.getBool(firebaseData, "/Network_Config/Active_Nodes/Node1/is_active")) {
          isActive = firebaseData.boolData();
      }

      // --- ĐỌC QUYẾT ĐỊNH AI CỦA NODE 1 ---
      if (Firebase.getString(firebaseData, "/Network_Config/Active_Nodes/Node1/ai_decision")) {
          ai_decision_S1 = firebaseData.stringData();
      }
      // -----------------------------------------------------

      if (isActive) {
        Humd[0] = 0; Temp[0] = 0.00; Light[0] = 0; SoilMoisture[0] = 0;
        sendMessage("", Destination_ESP32_Slave_1, get_Data_Mode);
      } else {
        Serial.println("[HỆ THỐNG] Node 1 đã bị khóa/xóa. Đang bỏ qua...");
      }
    }
    //:::::::::::::::::: 
    
    //:::::::::::::::::: Cập nhật Logic Quản lý Node 2
    if (Slv == 2) {
      bool isActive = true;
      if (Firebase.getBool(firebaseData, "/Network_Config/Active_Nodes/Node2/is_active")) {
          isActive = firebaseData.boolData();
      }

      // --- ĐỌC QUYẾT ĐỊNH AI CỦA NODE 2 ---
      if (Firebase.getString(firebaseData, "/Network_Config/Active_Nodes/Node2/ai_decision")) {
          ai_decision_S2 = firebaseData.stringData();
      }
      // -----------------------------------------------------

      if (isActive) {
        Humd[1] = 0; Temp[1] = 0.00; Light[1] = 0; SoilMoisture[1] = 0;
        sendMessage("", Destination_ESP32_Slave_2, get_Data_Mode);
      } else {
        Serial.println("[HỆ THỐNG] Node 2 đã bị khóa/xóa. Đang bỏ qua...");
      }
    }
    //:::::::::::::::::: 
  }
  //---------------------------------------- 

  //---------------------------------------- 
  if (finished_Sending_Message == true && finished_Receiving_Message == true) {
    if (send_Control_LED == true) {
      delay(250);
      send_Control_LED = false;
      if (Slave_Number == "S1") {
        Message = "";
        Message = LED_Number + "," + LED_Value;
        sendMessage(Message, Destination_ESP32_Slave_1, led_Control_Mode);
      }
      if (Slave_Number == "S2") {
        Message = "";
        Message = LED_Number + "," + LED_Value;
        sendMessage(Message, Destination_ESP32_Slave_2, led_Control_Mode);
      }
      delay(250);
    }
  }
  //---------------------------------------- 


  unsigned long currentMillis_RestartLORA = millis();
  
  if (currentMillis_RestartLORA - previousMillis_RestartLORA >= interval_RestartLORA) {
    previousMillis_RestartLORA = currentMillis_RestartLORA;

    count_to_Rst_LORA++;
    if (count_to_Rst_LORA > 30) {
      LoRa.end();
      Rst_LORA();
    }
  }
  //----------------------------------------parse for a packet, and call onReceive with the result:
  onReceive(LoRa.parsePacket());
  //----------------------------------------
}
