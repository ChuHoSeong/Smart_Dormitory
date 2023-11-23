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
} AirCleanerData; // 공기청정기 데이터 구조체

typedef struct {
  bool power;
  int temp;
  int humidity;
} HumidifierData; // 가습기 데이터 구조체

typedef struct {
  bool power[3];
} SwitchData; // 스위치 데이터 구조체

AirCleanerData airCleaner;
HumidifierData humidifier;
SwitchData switchControl;

RF24 radio(10, 9); // RF24 객체 (CE, CSN 핀 번호)

void setup() {
  Serial.begin(9600);
  radio.begin();
  updateDeviceData(); // 장치 데이터 업데이트
}

void loop() {
  Serial.println(analogRead(A0)); // 빛 센서 값 출력
  updateDeviceData(); // 장치 데이터 업데이트
  Serial.println("--------------------");
  delay(1000); // 1초 대기
}

// 각 장치의 데이터를 업데이트하는 함수들
void updateDeviceData() {
  updateAirCleanerData();
  updateHumidifierData();
  updateSwitchData();
}

void updateAirCleanerData() {
  sendDataAndReceiveResponse(AIR_CLEANER_ADDRESS, &airCleaner, sizeof(AirCleanerData), "Air Cleaner");
}

void updateHumidifierData(){
  sendDataAndReceiveResponse(HUMIDIFIER_ADDRESS, &humidifier, sizeof(HumidifierData), "Humidifier");
}

void updateSwitchData() {
  sendDataAndReceiveResponse(SWITCH_ADDRESS, &switchControl, sizeof(SwitchData), "Switch");
}

// 데이터 전송 및 응답 수신 함수
void sendDataAndReceiveResponse(const byte address[], void* data, size_t dataSize, const char* deviceName) {
  setSendMode(address);
  radio.write(data, dataSize);
  Serial.println(String(deviceName) + " data sent"); // 데이터 전송 완료 메시지 출력

  if (!receiveDataWithTimeout(address)) {
    Serial.println(String("*") + deviceName + " system is turned off"); // 시스템이 꺼져 있는 경우 메시지 출력
  }
  else {
    radio.read(data, dataSize);
    printDeviceData(data, deviceName); // 데이터 출력 함수 호출
  }
}

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
  while (millis() - startTime < 1000) { // 1초 동안 데이터 수신 대기
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
