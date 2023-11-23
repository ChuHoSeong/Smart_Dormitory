#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// 장치 주소 상수 정의
const byte AIR_CLEANER_ADDRESS[6] = "00001";
const byte HUMIDIFIER_ADDRESS[6] = "00002";
const byte SWITCH_ADDRESS[6] = "00003";

// 데이터 구조체 정의
typedef struct {
  bool power;
  int quality;
} AirCleanerData;

typedef struct {
  bool power;
  int temp;
  int humidity;
} HumidifierData;

typedef struct {
  bool power[3];
} SwitchData;

AirCleanerData airCleaner;
HumidifierData humidifier;
SwitchData switchControl;

RF24 radio(10, 9); // RF24 객체 (CE, CSN 핀 번호)

void setup() {
  Serial.begin(9600);
  radio.begin();
  updateDeviceData();
}

void loop() {
  Serial.println(analogRead(A0)); // 빛 센서 값 출력
  updateDeviceData();
  Serial.println("--------------------");
  delay(1000); // 1초 대기
}


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

void sendDataAndReceiveResponse(const byte address[], void* data, size_t dataSize, const char* deviceName) {
  setSendMode(address);
  radio.write(data, dataSize);
  Serial.println(String(deviceName) + " data sent");

  if (!receiveDataWithTimeout(address)) {
    Serial.println(String("*") + deviceName + " system is turned off");
  }
  else {
    radio.read(data, dataSize);
    printDeviceData(data, deviceName); // 데이터 출력 함수 호출
  }
}

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

bool receiveDataWithTimeout(const byte address[]) {
  unsigned long startTime = millis();
  setReceiveMode(address);
  while (millis() - startTime < 1000) {
    if (radio.available()) {
      return true;
    }
  }
  return false;
}

void setSendMode(const byte address[]){
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MIN);
  radio.stopListening();
  Serial.println("Send mode set");
}

void setReceiveMode(const byte address[]){
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();
  Serial.println("Receive mode set");
}
