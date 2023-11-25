#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Servo.h>

Servo servo1;
Servo servo2;
Servo servo3;

struct SERVO{
  Servo* servoPtr;
  int buttonPin;
  bool prv_value;
};

SERVO servos[3];




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

  servo1.attach(7);
  servo2.attach(6);
  servo3.attach(5);

  servos[0].servoPtr = &servo1;
  servos[1].servoPtr = &servo2;
  servos[2].servoPtr = &servo3;

  servos[0].buttonPin = 4;
  servos[1].buttonPin = 3;
  servos[2].buttonPin = 2;

  servos[0].prv_value = false;
  servos[1].prv_value = false;
  servos[2].prv_value = false;


  recivedData = {{0,0,0},0};
  switchControl = {{0,0,0},0};
  //processSwitchData();

  
  servo1.write(50);
}
void loop() {
  //버튼 누르면 recivedData.power[] 값 변경, switchControl.changed = true;
  //recivedData.power[0] = random(0,2);
  //recivedData.power[1] = random(0,2);
  //recivedData.power[2] = random(0,2);
  detectButton();
  setReceiveMode();
  if (radio.available()) {
    Serial.println("Data received");
    char request[4];
    radio.read(&request, sizeof(request));
    if(strcmp(request, "get")==0)
    {
      delay(500);
      setSendMode();
      radio.write(&switchControl, sizeof(SWI));
      switchControl.changed = false;
    }
    else if(strcmp(request, "put")==0)
    {
      setReceiveMode();
      radio.read(&recivedData, sizeof(SWI));
      processSwitchData();
    }
  }
}

void detectButton(){
  Serial.println(digitalRead(servos[0].buttonPin));
  for(int i = 0;i<3;i++)
  {
    int readValue = digitalRead(servos[i].buttonPin);
    if(servos[i].prv_value!=readValue&&readValue==false)
    { 
      switchControl.power[i] = !switchControl.power[i];
      switchControl.changed = true;
      servoAct(i, !switchControl.power[i]);
    }
    servos[i].prv_value = readValue;
  }
}
void servoAct(int i,bool p){
  if(p)
  {
    //i번 스위치 on
    servos[i].servoPtr->write(180);
    delay(300);
    servos[i].servoPtr->write(90);
    Serial.print(i);
    Serial.println(":turn on");
  }
  else
  {
    servos[i].servoPtr->write(0);
    delay(300);
    servos[i].servoPtr->write(90);
    Serial.print(i);
    Serial.println(":turn off");
  }
}

void processSwitchData() {
  for(int i = 0;i<3;i++)
  {
    Serial.print(switchControl.power[i]);
    Serial.println(recivedData.power[i]);
    if(switchControl.power[i] == recivedData.power[i])continue;
    else if(switchControl.power[i] < recivedData.power[i])
    {
      //i번 스위치 on
      servoAct(i, true);
    }
    else if(switchControl.power[i] > recivedData.power[i])
    {
      //i번 스위치 off
      servoAct(i, false);
    }
  }
  switchControl = recivedData;
  Serial.println("-----");
 }
void setSendMode(){
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MIN);
  radio.stopListening();
  //Serial.println("Send mode set");
}

void setReceiveMode(){
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();
  //Serial.println("Receive mode set");
}
