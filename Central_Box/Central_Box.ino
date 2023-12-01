// 보드 관련 라이브러리
#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif

// firebase 관련 라이브러리
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// 통신 관련 셋팅
#define WIFI_SSID "452"
#define WIFI_PASSWORD "00000452"
#define API_KEY "AIzaSyBy5y3roWIZD6bVH70McUOXtnEUVIMHup8"
#define DATABASE_URL "https://smart-dormitory-protaku-789615-default-rtdb.firebaseio.com/" 
// nRF24 모듈 관련 라이브러
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

//LCD 라이브러
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>


// 장치 주소 상수 정의
const byte AIR_CLEANER_ADDRESS[6] = "00001"; // 공기청정기 주소
const byte HUMIDIFIER_ADDRESS[6] = "00002"; // 가습기 주소
const byte SWITCH_ADDRESS[6] = "00003"; // 스위치 주소

RF24 radio(2, 0); // RF24 객체 생성 (CE, CSN 핀 번호 지정)

// 데이터 구조체 정의
typedef struct __attribute__((packed)){
  int16_t quality;
  bool power;
} AirCleanerData; // 공기청정기 데이터 구조체

typedef struct __attribute__((packed)){
  int16_t temp;
  int16_t humidity;
  bool power;
} HumidifierData; // 가습기 데이터 구조체

typedef struct {
  bool power[3];
} SwitchData; // 스위치 데이터 구조체

// 각 장치에 대한 데이터를 포함하는 구조체 정의
struct AIR{
  AirCleanerData center;
  AirCleanerData device;
  AirCleanerData cloude;
}airCleaner;

struct HUM{
  HumidifierData center;
  HumidifierData device;
  HumidifierData cloude;
}humidifier;

struct SWI{
  SwitchData center;
  SwitchData device;
  SwitchData cloude;
}switchControl;


// Firebase 객체
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool signupOK = false;

LiquidCrystal_I2C lcd(0x27, 16, 2); 

// 초기 설정 함수
void setup() {
  Serial.begin(9600); // 시리얼 통신 시작

  // 와이파이 통신 시
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else Serial.printf("%s\n", config.signer.signupError.message.c_str());
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  radio.begin(); // 라디오 통신 시작
  radio.setPALevel(RF24_PA_MIN);

  // I2C 통신 시작
  Wire.begin(D2, D1); // SDA, SCL
  // LCD 초기화
  lcd.init();
  // 백라이트 켜기
  lcd.backlight();
  // 첫 번째 줄에 텍스트 출력
  lcd.setCursor(0, 0);
  lcd.print("init completed");
  // 두 번째 줄에 텍스트 출력
  lcd.setCursor(0, 1);
  lcd.print("LCD 16x2 + I2C");

  //Serial.println(sizeof(airCleaner.device));
  //Serial.println(sizeof(humidifier.device));
  //Serial.println(sizeof(switchControl.device));

  // 각 장치에 대한 초기화 수행
  init(AIR_CLEANER_ADDRESS, airCleaner, "Air Cleaner");
  init(HUMIDIFIER_ADDRESS, humidifier, "Humidifier");
  init(SWITCH_ADDRESS, switchControl, "Switch");

  Serial.println("--init completed");
}

// 장치 데이터를 초기화하는 템플릿 함수
template<typename T>
void init(const byte address[], T &deviceData, const char* deviceName) {
  radio.openWritingPipe(address); // 쓰기 파이프 열기
  radio.openReadingPipe(0, address); // 읽기 파이프 열기
  // 장치에서 데이터를 가져옵니다.
  get(address, &deviceData.device, sizeof(deviceData.device), deviceName);
  // 장치의 'origin'과 'cloude' 상태를 현재 장치 상태로 초기화(동기화)합니다.
  deviceData.cloude = deviceData.device;
  deviceData.center = deviceData.device;

  
  Serial.println("init");
  printDeviceData(deviceData.center); 
  printDeviceData(deviceData.device); 
  printDeviceData(deviceData.cloude); 
  // 서버에 deviceData.cloude 값 업로드

  toFirebase(deviceData.cloude);
}

// 메인 루프 함수
void loop() {
  //Serial.println(analogRead(A0)); // 아날로그 핀에서 읽은 값을 출력

  // 공기청정기, 가습기, 스위치에 대해 템플릿 함수를 사용하여 업데이트합니다.
  syncData(AIR_CLEANER_ADDRESS, airCleaner, "Air Cleaner");
  syncData(HUMIDIFIER_ADDRESS, humidifier, "Humidifier");
  syncData(SWITCH_ADDRESS, switchControl, "Switch");
  printLcd();
  Serial.println("--------------------");
  delay(200); // 100ms 지연
}


// 각 장치 데이터를 업데이트하는 템플릿 함수
template<typename T>
void syncData(const byte address[], T &deviceData, const char* deviceName) {
  radio.openWritingPipe(address); // 쓰기 파이프 열기
  radio.openReadingPipe(0, address); // 읽기 파이프 열기
  // 장치에서 최신 데이터를 가져옵니다.
  get(address, &deviceData.device, sizeof(deviceData.device), deviceName);
  fromFirebase(deviceData.cloude, deviceName);

  Serial.println("get");
  printDeviceData(deviceData.center); 
  printDeviceData(deviceData.device); 
  printDeviceData(deviceData.cloude);

  if(checkChangePower(deviceData.center, deviceData.cloude)) {//서버에서 변경 사항이 없는 경우
    Serial.println("change detected cloude");
    deviceData.center = deviceData.cloude;
    deviceData.device = deviceData.cloude;
    put(address, &deviceData.device, sizeof(deviceData.device), deviceName);
  }
  else
  {
    // 상시 동기화
    Serial.println("non");
    deviceData.center = deviceData.device;
    deviceData.cloude = deviceData.device;
    // 서버에 deviceData.cloude 값을 업로드
    toFirebase(deviceData.cloude);
  }
}

template<typename T> bool checkChangePower(T &data1, T &data2){
  return (data1.power != data2.power);
}
template<> bool checkChangePower<SwitchData>(SwitchData &data1, SwitchData &data2){
  for(int i = 0;i<3;i++)
  {
    if(data1.power[i]!=data2.power[i])return true;
  }
  return false;
}

// 특정 주소의 장치로부터 데이터를 받아오는 함수
void get(const byte address[], void* data, size_t dataSize, const char* deviceName){
  
  radio.stopListening();
  
  char request[4] = "get"; // 'get' 요청 메시지
  radio.write(&request, sizeof(request)); // 요청 메시지 전송
  radio.startListening();
  delay(200);
  if(radio.available()){
    radio.read(data, dataSize); // 데이터 수신
    Serial.print("realtime_size = ");
    Serial.println(dataSize);
  }
  else 
  {
    Serial.println(String("*") + deviceName + " system is turned off"); // 시스템 꺼짐 메시지 출력
  } 
}

// 특정 주소의 장치에 데이터를 전송하는 함수
void put(const byte address[], void* data, size_t dataSize, const char* deviceName){
  radio.stopListening();// 데이터 전송 모드 설정
  char request[4] = "put"; // 'put' 요청 메시지
  radio.write(&request, sizeof(request)); // 요청 메시지 전송
  //Serial.println(String(deviceName) + " : put : write"); // 로그 출력
  delay(100); // 안전을 위한 지연
  radio.write(data, dataSize); // 데이터 전송
  //Serial.println(String(deviceName) + " : data : write"); // 로그 출력
}

// 장치 데이터를 출력하는 함수 -> 추후 LCD에 출력시키는 메소드로 변경하면 됨
template<typename T>
void printDeviceData(T &data){
  Serial.println("print : Unknown Device Type");
}
template<>
void printDeviceData<AirCleanerData>(AirCleanerData &data) {
  Serial.print("* Power: "); Serial.print(data.power);
  Serial.print(", Quality: "); Serial.println(data.quality);
  //Serial.print(", Changed: "); Serial.println(data.changed);
}
template<>
void printDeviceData<HumidifierData>(HumidifierData &data) {
  Serial.print("* Power: "); Serial.print(data.power);
  Serial.print(", Temp: "); Serial.print(data.temp);
  Serial.print(", Humidity: "); Serial.println(data.humidity);
  //Serial.print(", Changed: "); Serial.println(data.changed);
}
template<>
void printDeviceData<SwitchData>(SwitchData &data) {
  Serial.print("* Switch States: ");
  for (int i = 0; i < 3; i++) {
    Serial.print(data.power[i]); Serial.print(" ");
  }
  Serial.println();
  //Serial.print(", Changed: "); Serial.println(data.changed);
}

void printLcd(){
  lcd.setCursor(0, 0);
  lcd.print(humidifier.center.temp);
  lcd.write(byte(0xDF)); // '°' 기호 출력
  lcd.print("C ");
  lcd.print(humidifier.center.humidity);
  lcd.print("% ");
  lcd.print(airCleaner.center.quality);
  lcd.print("ug/m3");
  
  lcd.setCursor(0, 1);
  lcd.print("Sw1:");
  lcd.print(switchControl.center.power[0] ? "On" : "Off");
  lcd.print(" Sw2:");
  lcd.print(switchControl.center.power[1] ? "On" : "Off");
  lcd.print(" Sw3:");
  lcd.print(switchControl.center.power[2] ? "On" : "Off");
}

template<typename T>
bool fromFirebase(T &deviceData, const char *deviceName) {
  Serial.println("firebase : Unknown Device Type");
  return false;
}
template<>
bool fromFirebase<AirCleanerData>(AirCleanerData &deviceData, const char *deviceName) {
  // Firebase를 사용할 준비가 되지 않았다면, false 반환
  if (!Firebase.ready() || !signupOK) {
    Serial.println(fbdo.errorReason());
    return false;
  }

  // 기기명으로 DB의 객체에 접근
  // 만약 접근에 실패했다면 에러코드를 출력하며 false 반환
  if (Firebase.RTDB.getJSON(&fbdo, deviceName) == false) {
    Serial.println(fbdo.errorReason());
    return false;
  }
  
  // json으로 저장된 데이터를 센터에 저장하는 과정
  FirebaseJson json = fbdo.jsonObject();
  FirebaseJsonData data;
  json.get(data, "power");
  deviceData.power = data.to<bool>();
  json.get(data, "quality");
  deviceData.quality = data.to<int>();
  return true;
}
template<>
bool fromFirebase<HumidifierData>(HumidifierData &deviceData, const char *deviceName) {
  // Firebase를 사용할 준비가 되지 않았다면, false 반환
  if (!Firebase.ready() || !signupOK) {
    Serial.println(fbdo.errorReason());
    return false;
  }

  // 기기명으로 DB의 객체에 접근
  // 만약 접근에 실패했다면 에러코드를 출력하며 false 반환
  if (Firebase.RTDB.getJSON(&fbdo, deviceName) == false) {
    Serial.println(fbdo.errorReason());
    return false;
  }
  
  // json으로 저장된 데이터를 센터에 저장하는 과정
  FirebaseJson json = fbdo.jsonObject();
  FirebaseJsonData data;
  json.get(data, "power");
  deviceData.power = data.to<bool>();
  json.get(data, "humidity");
  deviceData.humidity = data.to<int>();
  json.get(data, "temp");
  deviceData.temp = data.to<int>();
  return true;
}
template<>
bool fromFirebase<SwitchData>(SwitchData &deviceData, const char *deviceName) {
  // Firebase를 사용할 준비가 되지 않았다면, false 반환
  if (!Firebase.ready() || !signupOK) {
    Serial.println(fbdo.errorReason());
    return false;
  }

  // 기기명으로 DB의 객체에 접근
  // 만약 접근에 실패했다면 에러코드를 출력하며 false 반환
  if (Firebase.RTDB.getJSON(&fbdo, deviceName) == false) {
    Serial.println(fbdo.errorReason());
    return false;
  }
  
  // json으로 저장된 데이터를 센터에 저장하는 과정
  FirebaseJson json = fbdo.jsonObject();
  FirebaseJsonData data;
  FirebaseJsonArray ary;
  json.get(data, "power");
  data.get<FirebaseJsonArray>(ary);
  for (size_t i=0 ; i < ary.size() ; i++) {
    ary.get(data, i);
    deviceData.power[i] = data.to<bool>();
  }
  return true;
}

template<typename T>
bool toFirebase(T &deviceData) {
  Serial.println("Unknown Device Type");
  return false;
}
template<>
bool toFirebase<AirCleanerData>(AirCleanerData &deviceData) {
  // Firebase를 사용할 준비가 되지 않았다면, false 반환
  if (!Firebase.ready() || !signupOK) {
    Serial.println(fbdo.errorReason());
    return false;
  }

  // 센터의 데이터를 json으로 변환하는 과정
  FirebaseJson json;
  json.set("/Air Cleaner/power", deviceData.power);
  json.set("/Air Cleaner/quality", deviceData.quality);
  if (Firebase.RTDB.updateNode(&fbdo, "/", &json) == false) {
    Serial.println(fbdo.errorReason());
    return false;    
  }
  return true;
}
template<>
bool toFirebase<HumidifierData>(HumidifierData &deviceData) {
  // Firebase를 사용할 준비가 되지 않았다면, false 반환
  if (!Firebase.ready() || !signupOK) {
    Serial.println(fbdo.errorReason());
    return false;
  }

  // 센터의 데이터를 json으로 변환하는 과정
  FirebaseJson json;
  json.set("/Humidifier/power", deviceData.power);
  json.set("/Humidifier/humidity", deviceData.humidity);
  json.set("/Humidifier/temp", deviceData.temp);
  if (Firebase.RTDB.updateNode(&fbdo, "/", &json) == false) {
    Serial.println(fbdo.errorReason());
    return false;    
  }
  return true;
}
template<>
bool toFirebase<SwitchData>(SwitchData &deviceData) {
  // Firebase를 사용할 준비가 되지 않았다면, false 반환
  if (!Firebase.ready() || !signupOK) {
    Serial.println(fbdo.errorReason());
    return false;
  }

  // 센터의 데이터를 json으로 변환하는 과정
  FirebaseJson json;
  for (int i=0 ; i < 3 ; i++)
    json.set("/Switch/power/" + String(i), deviceData.power[i]);
  if (Firebase.RTDB.updateNode(&fbdo, "/", &json) == false) {
    Serial.println(fbdo.errorReason());
    return false;    
  }
  return true;
}