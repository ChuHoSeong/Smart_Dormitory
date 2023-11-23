#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(10, 9);
const byte address[6] = "00001";

typedef struct {
  bool power;
  int quality;
} AIR;

AIR airCleaner = {
  true,
  0,
};

void setup() {
  Serial.begin(9600);
  radio.begin();
}

void loop() {
  setReceiveMode();
  if (radio.available()) {
    Serial.println("Data received");
    radio.read(&airCleaner, sizeof(AIR));
    processAirData();
    setSendMode();
    radio.write(&airCleaner, sizeof(AIR));
  }
}

void processAirData() {
  airCleaner.quality = random(15, 30); // 공기질 알아서 잘 넣어주십쇼 희근씨 화이팅
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
