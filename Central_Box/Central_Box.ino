#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// 장치 주소 상수 정의
const byte AIR_CLEANER_ADDRESS[6] = "00001"; // 공기청정기 주소
const byte HUMIDIFIER_ADDRESS[6] = "00002"; // 가습기 주소
const byte SWITCH_ADDRESS[6] = "00003"; // 스위치 주소

// 데이터 구조체 정의
typedef struct {
  bool power;
  int quality;
  bool changed;
} AirCleanerData; // 공기청정기 데이터 구조체

typedef struct {
  bool power;
  int temp;
  int humidity;
  bool changed;
} HumidifierData; // 가습기 데이터 구조체

typedef struct {
  bool power[3];
  bool changed;
} SwitchData; // 스위치 데이터 구조체

struct AIR{
  AirCleanerData origin;
  AirCleanerData device;
  AirCleanerData cloude;
}airCleaner;
struct HUM{
  HumidifierData origin;
  HumidifierData device;
  HumidifierData cloude;
}humidifier;
struct SWI{
  SwitchData origin;
  SwitchData device;
  SwitchData cloude;
}switchControl;

RF24 radio(10, 9); // RF24 객체 (CE, CSN 핀 번호)

void setup() {
  Serial.begin(9600);
  radio.begin();
  init(AIR_CLEANER_ADDRESS, &airCleaner, sizeof(AirCleanerData), "Air Cleaner");
  init(HUMIDIFIER_ADDRESS, &humidifier, sizeof(HumidifierData), "Humidifier");
  init(SWITCH_ADDRESS, &switchControl, sizeof(SwitchData), "Switch");
}

void init(const byte address[], void* data, size_t dataSize, const char* deviceName) {
  get(address, data, dataSize, deviceName);
  // Casting void* to appropriate data type
  if (strcmp(deviceName, "Air Cleaner") == 0) {
    ((AIR*)data)->origin = ((AIR*)data)->device;
    ((AIR*)data)->cloude = ((AIR*)data)->device;
  } else if (strcmp(deviceName, "Humidifier") == 0) {
    ((HUM*)data)->origin = ((HUM*)data)->device;
    ((HUM*)data)->cloude = ((HUM*)data)->device;
  } else if (strcmp(deviceName, "Switch") == 0) {
    ((SWI*)data)->origin = ((SWI*)data)->device;
    ((SWI*)data)->cloude = ((SWI*)data)->device;
  }
}

void loop() {
  Serial.println(analogRead(A0)); // Print light sensor value

  updateAirCleanerData();
  updateHumidifierData();
  updateSwitchData();
  Serial.println("--------------------");
  delay(100); // Wait for 1 second
}
void get(const byte address[], void* data, size_t dataSize, const char* deviceName){
  setSendMode(address);
  char request[4] = "get";
  radio.write(&request, sizeof(request));
  Serial.println(String(deviceName) + " : get : write"); // 데이터 전송 완료 메시지 출력

  if (!receiveDataWithTimeout(address)) {
    Serial.println(String("*") + deviceName + " system is turned off"); // 시스템이 꺼져 있는 경우 메시지 출력
  }
  else {
    radio.read(data, dataSize);
    printDeviceData(data, deviceName); // 데이터 출력 함수 호출
  }
}
void put(const byte address[], void* data, size_t dataSize, const char* deviceName){
  setSendMode(address);
  char request[4] = "put";
  radio.write(&request, sizeof(request));
  Serial.println(String(deviceName) + " : put : write"); // 데이터 전송 완료 메시지 출력
  delay(100);//혹시몰라서 딜레이
  setSendMode(address);
  radio.write(&data, dataSize);
  Serial.println(String(deviceName) + " : data : write"); 
}
void updateAirCleanerData(){
  get(AIR_CLEANER_ADDRESS, &airCleaner.device, sizeof(AirCleanerData), "Air Cleaner");
  /*get : 서버에서 데이터 가져오와  airCleaner.cloude에 넣기*/
  if(airCleaner.device.changed)
  {
    //device >> origin >> cloude
    airCleaner.origin = airCleaner.device;
    airCleaner.cloude = airCleaner.device;
    //put(AIR_CLEANER_ADDRESS, &airCleaner.device, sizeof(AirCleanerData), "Air Cleaner");
  }
  else if (airCleaner.cloude.changed)
  {
    //cloude >> origin >> device
    airCleaner.origin =   airCleaner.cloude;
    airCleaner.device =   airCleaner.cloude;
  }
  else
  {
    Serial.println("[no changed]");
  }
  airCleaner.device.changed = false;
  airCleaner.cloude.changed = false;
  put(AIR_CLEANER_ADDRESS, &airCleaner.device, sizeof(AirCleanerData), "Air Cleaner");
  /*put : 서버에  airCleaner.cloude 업로드*/
}
void updateHumidifierData(){
  get(HUMIDIFIER_ADDRESS, &humidifier.device, sizeof(HumidifierData), "Humidifier");
  /*get : 서버에서 데이터 가져오와  humidifier.cloude에 넣기*/
  if(humidifier.device.changed)
  {
    //device >> origin >> cloude
    humidifier.origin = humidifier.device;
    humidifier.cloude = humidifier.device;
  }
  else if (humidifier.cloude.changed)
  {
    //cloude >> origin >> device
    humidifier.origin = humidifier.cloude;
    humidifier.device = humidifier.cloude;
  }
  else
  {
    Serial.println("[no changed]");
  }
  humidifier.device.changed = false;
  humidifier.cloude.changed = false;
  put(HUMIDIFIER_ADDRESS, &humidifier.device, sizeof(HumidifierData), "Humidifier");
  /*put : 서버에  humidifier.cloude 업로드*/
}
void updateSwitchData() {
  get(SWITCH_ADDRESS, &switchControl.device, sizeof(SwitchData), "Switch");
  /*get : 서버에서 데이터 가져오와  switchControl.cloude에 넣기*/
  if(switchControl.device.changed)
  {
    //device >> origin >> cloude
    switchControl.origin = switchControl.device;
    switchControl.cloude = switchControl.device;
  }
  else if (switchControl.cloude.changed)
  {
    //cloude >> origin >> device
    switchControl.origin =  switchControl.cloude;
    switchControl.device =  switchControl.cloude;
  }
  else
  {
    Serial.println("[no changed]");
  }
  switchControl.device.changed = false;
  switchControl.cloude.changed = false;
  put(SWITCH_ADDRESS, &switchControl.device, sizeof(SwitchData), "Switch");
  /*put : 서버에  switchControl.cloude 업로드*/
}

// 데이터 전송 및 응답 수신 함수


// 장치 데이터를 출력하는 함수
void printDeviceData(void* data, const char* deviceName) {
  if (strcmp(deviceName, "Air Cleaner") == 0) {
    AirCleanerData* acData = (AirCleanerData*)data;
    Serial.print("* Power: "); Serial.print(acData->power);
    Serial.print(", Quality: "); Serial.println(acData->quality);
  } 
  else if (strcmp(deviceName, "Humidifier") == 0) {
    HumidifierData* humData = (HumidifierData*)data;
    Serial.print("* Power: "); Serial.print(humData->power);
    Serial.print(", Temp: "); Serial.print(humData->temp);
    Serial.print(", Humidity: "); Serial.println(humData->humidity);
  }
  else if (strcmp(deviceName, "Switch") == 0) {
    SwitchData* swData = (SwitchData*)data;
    Serial.print("* Switch States: ");
    for (int i = 0; i < 3; i++) {
      Serial.print(swData->power[i]); Serial.print(" ");
    }
    Serial.println();
  }
}

// 타임아웃을 설정하여 데이터 수신 여부를 확인하는 함수
bool receiveDataWithTimeout(const byte address[]) {
  unsigned long startTime = millis();
  setReceiveMode(address);
  while (millis() - startTime < 500) { // 0.5초 동안 데이터 수신 대기
    if (radio.available()) {
      return true;
    }
  }
  return false;
}

// 데이터 전송 모드 설정 함수
void setSendMode(const byte address[]){
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MIN);
  radio.stopListening();
  Serial.println("Send mode set"); // 전송 모드 설정 완료 메시지 출력
}

// 데이터 수신 모드 설정 함수
void setReceiveMode(const byte address[]){
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();
  Serial.println("Receive mode set"); // 수신 모드 설정 완료 메시지 출력
}
