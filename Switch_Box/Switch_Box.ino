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
  int ledPin;
  bool prv_value;
};

SERVO servos[3] = {
  {&servo1, 4, A0, false},
  {&servo2, 3, A1, false},
  {&servo3, 2, A2, false}
};

RF24 radio(10, 9);
const byte address[6] = "00003";

typedef struct {
  bool power[3];
} DATA;

DATA currentData;
DATA receivedData;

void setup() {
  Serial.begin(9600);
  radio.begin();
  radio.setPALevel(RF24_PA_HIGH);
  radio.openWritingPipe(address);
  radio.openReadingPipe(0, address);
  radio.startListening();

  servo1.attach(7);
  servo2.attach(6);
  servo3.attach(5);

  currentData = {{0,0,0}};
  receivedData = {{0,0,0}};
  //processSwitchData();

  servo1.write(90);
}
void loop() {
  detectButton();
  detectSignal();
  actByData();
}

void detectButton(){
  //Serial.println(digitalRead(servos[0].buttonPin));
  for(int i = 0;i<3;i++)
  {
    bool readValue = digitalRead(servos[i].buttonPin);
    if(servos[i].prv_value!=readValue&&readValue==false)
    { 
      currentData.power[i] = !currentData.power[i];
    }
    servos[i].prv_value = readValue;
  }
}
void detectSignal(){
  if (radio.available()) {
    Serial.println("Data received");
    char request[4];
    radio.read(&request, sizeof(request));
    if(strcmp(request, "get")==0)
    {
      radio.stopListening();
      delay(100);
      radio.write(&currentData, sizeof(currentData));
      radio.startListening();
    }
    else if(strcmp(request, "put")==0)
    {
      delay(200);
      if(radio.available())radio.read(&currentData, sizeof(currentData));
    }
  }
}

void actByData() {
  for(int i = 0;i<3;i++)
  {
    actServo(i,currentData.power[i]);
  }
  Serial.println("\n-----");
 }

void actServo(int i,bool p){
  if(p)
  {
    servos[i].servoPtr->write(110);
    Serial.print(i);
    Serial.print(":O ");
  }
  else
  {
    servos[i].servoPtr->write(70);
    Serial.print(i);
    Serial.print(":X ");
  }
  analogWrite(servos[i].ledPin, 150*(!p));
}
