#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(10, 9);
const byte address[6] = "00003";

typedef struct {
  bool power[3];
  bool changed;
} SWI;


SWI switchControl;
SWI recivedData;

void setup() {
  Serial.begin(9600);
  radio.begin();

  recivedData = {{0,0,0},0};
  switchControl = {{0,0,0},0};
  //processSwitchData();
}
void loop() {
  //스위치감지
  //버튼 누르면 recivedData.power[] 값 변경, switchControl.changed = true;
  recivedData.power[0] = random(0,2);
  recivedData.power[1] = random(0,2);
  recivedData.power[2] = random(0,2);
  
  processSwitchData();
  
  switchControl = recivedData;
  setReceiveMode();
  if (radio.available()) {
    Serial.println("Data received");
    char request[4];
    radio.read(&request, sizeof(request));
    if(strcmp(request, "get")==0)
    {
      setSendMode();
      radio.write(&switchControl, sizeof(SWI));
      //switchControl.changed = false;
    }
    else if(strcmp(request, "put")==0)
    {
      setReceiveMode();
      radio.read(&recivedData, sizeof(SWI));
    }
  }
}

void processSwitchData() {
  for(int i = 0;i<3;i++)
  {
    if(switchControl.power[i] == recivedData.power[i])continue;
    else if(switchControl.power[i] < recivedData.power[i])
    {
      //i번 스위치 on
    }
    else if(switchControl.power[i] > recivedData.power[i])
    {
      //i번 스위치 off
    }
    switchControl.power[i] = recivedData.power[i];
    switchControl.changed = true;
  }
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
