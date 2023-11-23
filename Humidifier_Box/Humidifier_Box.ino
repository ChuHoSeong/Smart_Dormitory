#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(10, 9);
const byte address[6] = "00002";

typedef struct {
  bool power;
  int temp;
  int humidity;
} HUM;

HUM humidifier = {
  true,
  0,
  0
};

void setup() {
  Serial.begin(9600);
  radio.begin();
}

void loop() {
  setReceiveMode();
  if (radio.available()) {
    Serial.println("Data received");
    radio.read(&humidifier, sizeof(HUM));
    processHumidifityData();
    setSendMode();
    radio.write(&humidifier, sizeof(HUM));
  }
}

void processHumidifityData() {
  // 랜덤한 온도와 습도 값을 생성합니다.
  humidifier.temp = random(15, 30); // 온도를 15 ~ 30도 사이로 설정
  humidifier.humidity = random(30, 80); // 습도를 30% ~ 80% 사이로 설정
}

void setSendMode(){
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MIN);
  radio.stopListening();
  Serial.println("Send mode set");
}

void setReceiveMode(){
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();
  Serial.println("Receive mode set");
}
