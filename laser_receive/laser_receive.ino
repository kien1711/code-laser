//MAC this slave: 0C:DC:7E:63:41:30

#include <esp_now.h>
#include <WiFi.h>
#include<math.h>

#define CW HIGH
#define CCW LOW

#define En2 26
#define En1 27

#define buttonPin18 18  // GPIO18 - Chân nút nhấn
#define buttonPin19 19 // GPIO19 - Chân nút nhấn
#define led_in_board 2 // GPIO2 - Chân LED
#define laser 25        //laser

#define dirNgangPin 12
#define StepNgangPin 13
#define dirDocPin 14
#define StepDocPin 15
#define speed 1500 // Tốc độ mặc định

#define speed_home 500

#define TiLeBuoc 16

volatile bool value1 = true; // Biến cờ hiệu cho biết trạng thái
volatile bool value2 = true; // Biến cờ hiệu cho biết trạng thái

bool laserOn = false; // Biến kiểm tra trạng thái laser
unsigned long previousMillis = 0; // Thời gian trước đó
const long interval = 300000; // Thời gian chờ (10 giây)
unsigned long currentMillis ;

bool chieu_doc, chieu_ngang;
int xung_doc , xung_ngang;

uint8_t broadcastAddress[] = {0x08, 0xD1, 0xF9, 0xE9, 0xB2, 0xA8};
char data[32];
esp_now_peer_info_t peerInfo;

void dk_dc_ngang(int *step, bool ChieuQuay);
void dk_dc_doc(int *step, bool ChieuQuay);
void Go_Home();
void laser_time();
void Quay_Laser(bool ChieuDoc, int *XungDoc, bool ChieuNgang, int *XungNgang);

void IRAM_ATTR buttonISR18() {
  value1 = false; // Đánh dấu rằng nút GPIO18 đã được nhấn
}

void IRAM_ATTR buttonISR19() {
  value2 = false; // Đánh dấu rằng nút GPIO19 đã được nhấn);
}
void goHome() {
  value1 = value2 = true; // Đánh dấu rằng nút GPIO19 đã được nhấn
  Go_Home();
}

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  char *buff = (char *)data;
  String input = String(buff);
  Serial.println(input);
  if (input.charAt(0) == 'r')
    {
      // ESP.restart();
       goHome();
    }
      
    /////////////////// chuỗi: L_0/1_100_0/1_100   
    // ký từ đầu: L
    // 2 số đầu: chiều và xung động cơ dưới
    // 2 số sau: chiều và xung động cơ trên

    //xử lý chuỗi
    int spaceIndex1 = input.indexOf('_'); // Tìm vị trí shift gạch 1
    int spaceIndex2 = input.indexOf('_', spaceIndex1 + 1); // Tìm vị trí shift gạch  thứ hai
    int spaceIndex3 = input.indexOf('_', spaceIndex2 + 1); // Tìm vị trí shift gạch  thứ ba
    int spaceIndex4 = input.indexOf('_', spaceIndex3 + 1); // Tìm vị trí shift gạch  thứ ba
    if(input.charAt(0) == 'L')
    {
      if (spaceIndex1 != -1 && spaceIndex2 != -1 && spaceIndex3 != -1 && spaceIndex4 != -1) 
      {
//        input.substring(0, spaceIndex1).toCharArray(kytu_dau, 5); // ký tự đầu của chuỗi
        chieu_doc = input.substring(spaceIndex1 + 1).toInt(); // Lấy số bước từ chuỗi
        xung_doc = input.substring(spaceIndex2 + 1).toInt(); // Lấy số bước từ chuỗi
        chieu_ngang = input.substring(spaceIndex3 + 1).toInt(); // Lấy số bước từ chuỗi
        xung_ngang = input.substring(spaceIndex4 + 1).toInt(); // Lấy số bước từ chuỗi
        
        Serial.print("chuỗi data nhận được: ");
        Serial.println(input);
        // Serial.print("chiều dọc: ");
        // Serial.println(chieu_doc);
        // Serial.print("xung dọc: ");
        // Serial.println(xung_doc);
        // Serial.print("chiều ngang: ");
        // Serial.println(chieu_ngang);
        // Serial.print("xung ngang: ");
        // Serial.println(xung_ngang);
      }
    }
    // Quay động cơ
    Quay_Laser(chieu_doc, &xung_doc, chieu_ngang, &xung_ngang);

    //SAU KHI QUAY XONG SẼ GỬI KÝ TỰ BÁO XONG
    char dataSend[] = "LaserHome";
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)dataSend, sizeof(dataSend));
     
    if (result == ESP_OK) {
      Serial.println("Sent with success");
    }
    else {
      Serial.println("Error sending the data");
    }
}
 void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  Serial.setTimeout(10);
  Serial.print("start...");
  pinMode(buttonPin18, INPUT_PULLUP); // Cấu hình chân nút nhấn với pull-up
  pinMode(buttonPin19, INPUT_PULLUP); // Cấu hình chân nút nhấn với pull-up
  pinMode(led_in_board, OUTPUT);            // Cấu hình chân LED là đầu ra
  pinMode(laser, OUTPUT);            // Cấu hình chân out laser
  pinMode(dirNgangPin, OUTPUT);
  pinMode(StepNgangPin, OUTPUT);
  pinMode(dirDocPin, OUTPUT);
  pinMode(StepDocPin, OUTPUT);
  pinMode(En1, OUTPUT);
  pinMode(En2, OUTPUT);
  
  digitalWrite(dirDocPin, CCW);
  digitalWrite(dirNgangPin, CW);

  attachInterrupt(digitalPinToInterrupt(buttonPin18), buttonISR18, FALLING);
  attachInterrupt(digitalPinToInterrupt(buttonPin19), buttonISR19, FALLING);
//  attachInterrupt(digitalPinToInterrupt(button21_home), buttonISR21, FALLING);
  // Gắn ngắt ngoài với chân nút nhấn (sự kiện FALLING - từ HIGH xuống LOW)
  digitalWrite(laser, 1);
  Serial.print("set up go home...");
  Go_Home();

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(OnDataRecv);
  esp_now_register_send_cb(OnDataSent); 
  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;  
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
}
 
void loop() {
  
  laser_time();

}
void Go_Home()
{
  Serial.print("đang trong hàm home");
    digitalWrite(laser, HIGH);
    laserOn = true;
    previousMillis = millis(); // Lưu thời gian hiện tại
 ///////////////////// động cơ dưới go home
  for (int i = 0; i < 50; i++)
  {
    digitalWrite(dirDocPin, CCW);
    digitalWrite(StepDocPin, HIGH);
    delayMicroseconds(speed_home); // Delay theo tốc độ
    digitalWrite(StepDocPin, LOW);
    delayMicroseconds(speed_home);
  }
  delay(50);
    while (value2) {
    digitalWrite(dirDocPin, CW);
    digitalWrite(StepDocPin, HIGH);
    delayMicroseconds(speed_home); // Delay theo tốc độ
    digitalWrite(StepDocPin, LOW);
    delayMicroseconds(speed_home);
  }  
  delay(50);
  for (int i = 0; i < 786; i++)
  {
    digitalWrite(dirDocPin, CCW);
    digitalWrite(StepDocPin, HIGH);
    delayMicroseconds(speed_home); // Delay theo tốc độ
    digitalWrite(StepDocPin, LOW);
    delayMicroseconds(speed_home);
  }
  delay(50);
  
  /////////////////////////// động cơ trên go home
  for (int i = 0; i < 50; i++)
  {
    digitalWrite(dirNgangPin, CCW);
    digitalWrite(StepNgangPin, HIGH);
    delayMicroseconds(speed_home); // Delay theo tốc độ
    digitalWrite(StepNgangPin, LOW);
    delayMicroseconds(speed_home);
  }
  delay(50);
  while (value1) {
    digitalWrite(dirNgangPin, CW);
    digitalWrite(StepNgangPin, HIGH);
    delayMicroseconds(speed_home); // Delay theo tốc độ
    digitalWrite(StepNgangPin, LOW);
    delayMicroseconds(speed_home);
  }
  delay(50);

  for (int i = 0; i < 100; i++)
  {
    digitalWrite(dirNgangPin, CCW);
    digitalWrite(StepNgangPin, HIGH);
    delayMicroseconds(speed_home); // Delay theo tốc độ
    digitalWrite(StepNgangPin, LOW);
    delayMicroseconds(speed_home);
  }
}

void dk_dc_ngang(int *step, bool ChieuQuay)
{
  digitalWrite(dirNgangPin,ChieuQuay);
  for (int i = 0; i < (*step) * TiLeBuoc; *step = *step - 1)
  {
    digitalWrite(StepNgangPin, HIGH);
    delayMicroseconds(speed); // Delay theo tốc độ
    digitalWrite(StepNgangPin, LOW);
    delayMicroseconds(speed);
  }
}
void dk_dc_doc(int *step, bool ChieuQuay)
{
  digitalWrite(dirDocPin, ChieuQuay);
  for (int i = 0; i < (*step) * TiLeBuoc; *step = *step - 1)
  {
    digitalWrite(StepDocPin, HIGH);
    delayMicroseconds(speed); // Delay theo tốc độ
    digitalWrite(StepDocPin, LOW);
    delayMicroseconds(speed);
  }
}
void laser_time()
{
      currentMillis = millis();
    if (laserOn && (currentMillis - previousMillis >= interval)) {
        // Tắt laser
        digitalWrite(laser, LOW);
        laserOn = false;
    }
}
void Quay_Laser(bool ChieuDoc, int *XungDoc, bool ChieuNgang, int *XungNgang)
{
    dk_dc_doc(XungDoc, ChieuDoc);
    dk_dc_ngang(XungNgang, ChieuNgang);
}
