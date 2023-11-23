#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(10, 9);
const byte address[6] = "00003";

typedef struct {
  bool power[3];
} SWI;


SWI switchControl = {
  {1,0,1}
};

void setup() {
  Serial.begin(9600);
  radio.begin();
}

void loop() {
  setReceiveMode();
  if (radio.available()) {
    Serial.println("Data received");
    radio.read(&switchControl, sizeof(SWI));
    processSwitchData();
    setSendMode();
    radio.write(&switchControl, sizeof(SWI));
  }
}

void processSwitchData() {
  switchControl.power[0] = random(0, 1); //여기에 스위치 on/off여부 정보를 넣어주면 됩니다. 잉요 화이팅
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
