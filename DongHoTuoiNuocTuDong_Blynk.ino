#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID           "TMPL6H_YFd18o"
#define BLYNK_TEMPLATE_NAME         "iotlabVNM"
#define BLYNK_AUTH_TOKEN            ""

//Khai báo thư viện
#include <WiFi.h>
#include "EEPROM.h"
#include <BlynkMultiClient.h>
#include <TimeLib.h>
#include <WidgetRTC.h>

//Định nghĩa, khai báo biến
#define LENGTH(x) (strlen(x) + 1)  
#define EEPROM_SIZE 200             
#define PIN_BUTTON 23   
#define PinOut 18
#define timeShow V1

String ssid;                       
String password;

unsigned long millis_RESET;
static WiFiClient blynkWiFiClient;
WidgetRTC rtc;
BlynkTimer timer;
int startHour, startMinute, stopHour, stopMinute, startHour2, startMinute2, stopHour2, stopMinute2;
int oldMinute, nowMinute;
bool pinValue, SW, oldSW = false;

//Kiểm tra thời gian giữ nút nhấn
  bool longPress(int t)
  {
    static int lastPress = 0;
    if(millis() - lastPress > t && digitalRead(PIN_BUTTON) == 1){
      return true;
    } else if (digitalRead(PIN_BUTTON) == 0) {
      lastPress = millis();
    }
    return false;
  }
//Ghi EEPROM
  void write_flash(const char* toStore, int startAddr) {
    int i = 0;
    for (; i < LENGTH(toStore); i++) {
      EEPROM.write(startAddr + i, toStore[i]);
    }
    EEPROM.write(startAddr + i, '\0');
    EEPROM.commit();
  }
//Đọc EEPROM
  String read_flash(int startAddr) {
    char in[128]; 
    int i = 0;
    for (; i < 128; i++) {
      in[i] = EEPROM.read(startAddr + i);
    }
    return String(in);
  }

//Kết nối Wifi
  void connect_Wifi()
  {
    //Kiem tra nut nhan GPIO23
    if(longPress(5000))
    {
      //Có nhấn, xóa EEPROM
      Serial.println("Reseting the WiFi credentials");
      write_flash("", 0); 
      write_flash("", 40); 
      Serial.println("Wifi credentials erased");
      Serial.println("Restarting the ESP");
      delay(500);
      ESP.restart();
    }
    else
    {
      //Không nhấn, đọc EEPROM
      ssid = read_flash(0);       //Gán SSID wifi vào ssid
      Serial.print("SSID = ");
      Serial.println(ssid);
      password = read_flash(40);  //Gán Password wifi vào ssid
      Serial.print("Password = ");
      Serial.println(password);
    }
    //Kết nối wifi
    WiFi.begin(ssid.c_str(), password.c_str());
    delay(3500);   

    //Kiểm tra trạng thái kết nối wifi
    if (WiFi.status() != WL_CONNECTED) 
    { 
      //Kết nối không thành công, chuyển độ WIFI_AP_STA
      WiFi.mode(WIFI_AP_STA);
      WiFi.beginSmartConfig(); //Bắt đầu kết nối wifi thông minh
      
      Serial.println("Waiting for SmartConfig.");
      while (!WiFi.smartConfigDone()) {
        delay(500);
        Serial.print(".");
      }
      Serial.println("");
      Serial.println("SmartConfig received.");
      
      Serial.println("Waiting for WiFi");
      //Kiểm tra trạng thái kết nối wifi
      while (WiFi.status() != WL_CONNECTED) {
        //Kết nối không thành công, đợi kết nối lại
        delay(500);
        Serial.print(".");
      }
      
      //Lưu dữ liệu EEPROM
      ssid = WiFi.SSID();
      password = WiFi.psk();
      Serial.print("SSID:");
      Serial.println(ssid);
      Serial.print("password:");
      Serial.println(password);
      Serial.println("Store SSID & password in Flash");
      write_flash(ssid.c_str(), 0); 
      write_flash(password.c_str(), 40); 
    }
    //Kết nối thành công
      Serial.println("WiFi Connected.");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
  }

//Bật tắt tưới lan tại vị trí thiết bị
  void check_Button()
  {
    SW = longPress(200);
    //So sánh trạng thái trước đó
    //Có sự thay đổi, trạng thái hiện tại = 1
    if(oldSW != SW && SW == 1) 
    {
      //Cập nhập trạng nút nhấn
      oldSW = SW;
      //Đọc, đảo trạng thái ngõ ra hiện tại
      pinValue = !digitalRead(PinOut);
      //Xuất giá trị ngõ ra
      digitalWrite(PinOut,   pinValue ? HIGH : LOW);
      //Cập nhập giá trị blynk
      Blynk.virtualWrite(V4, pinValue ?    1 : 0);
      Serial.println(pinValue ?    "Đèn bật" : "Đèn tắt");
    }
    if(oldSW != SW && SW == 0)
    {
      oldSW = SW;
    }
  }

// Đồng bộ thời gian khi kết nối
  BLYNK_CONNECTED() {
    rtc.begin();
    Blynk.syncAll();
  }

  String twoDigits(int digits) 
  { 
    if(digits < 10) return "0" + String(digits); 
    else return String(digits); 
  }

//Bật tắt tưới lan từ xa với blynk
  BLYNK_WRITE(V4)
  {
    pinValue = param.asInt();
    digitalWrite(PinOut, pinValue ? HIGH : LOW);
    Serial.println(pinValue ? "Đèn bật V4" : "Đèn tắt V4");
  }

//Lập lịch tưới lan với blynk
  BLYNK_WRITE(V2) {
    // Đọc giá trị time input
    Serial.println("Time Input 1");
    TimeInputParam t(param);

    if (t.hasStartTime()) {
      startHour = t.getStartHour();
      startMinute = t.getStartMinute();
    }

    if (t.hasStopTime()) {
      stopHour = t.getStopHour();
      stopMinute = t.getStopMinute();
    }
  }

  BLYNK_WRITE(V3) {
    // Xử lý đầu vào thời gian từ Blynk
    Serial.println("Time Input 2");
    TimeInputParam t(param);

    // Cập nhật thời gian bắt đầu và kết thúc
    if (t.hasStartTime()) {
      startHour2 = t.getStartHour();
      startMinute2 = t.getStartMinute();
    }

    if (t.hasStopTime()) {
      stopHour2 = t.getStopHour();
      stopMinute2 = t.getStopMinute();
    }
  }

  void checkTime() {
    nowMinute = minute(); 
    if(oldMinute != nowMinute)
    {
      String currentTime; 
      currentTime = twoDigits(hour()) + ":" + twoDigits(minute()); 
      Blynk.virtualWrite(timeShow, currentTime);
    }
    
    if (year() != 1970) { // Kiểm tra nếu thời gian đã được đồng bộ hóa
      // Lấy thời gian hiện tại
      int currentHour = hour();
      int currentMinute = minute();

      bool shouldBeOn = false;

      if ((currentHour > startHour || (currentHour == startHour && currentMinute >= startMinute)) &&
          (currentHour < stopHour  || (currentHour == stopHour && currentMinute < stopMinute))) {
        shouldBeOn = true;
      }

      if (shouldBeOn != pinValue) {
        pinValue = shouldBeOn;
        digitalWrite(PinOut, pinValue ? HIGH : LOW);
        Blynk.virtualWrite(V4, pinValue ? 1 : 0);
        Serial.println(pinValue ? "Đèn bật" : "Đèn tắt");
      }
    }
  }
void setup() {
  //Khởi tạo
  millis_RESET = millis();
  Serial.begin(115200);
  pinMode(PIN_BUTTON,   INPUT);
  pinMode(PinOut,      OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  //Kết nối wifi
  connect_Wifi();
  //Kết nối blynk
  Blynk.addClient("WiFi", blynkWiFiClient, 80);
  Blynk.config(BLYNK_AUTH_TOKEN);
  timer.setInterval(10000L, checkTime);
}

void loop() {
  check_Button();
  Blynk.run();
}
