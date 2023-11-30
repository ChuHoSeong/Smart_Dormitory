#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <math.h>

RF24 radio(10, 9);
const byte address[6] = "00001";

typedef struct {
  int quality;
  bool power;
} DATA;

DATA currentData;

int button = 4;//임의로 7로 정함
int fan1 = 7;
int fan2 = 6;
int led = 5;
int r = A3;
int g = A4;
int b = A5;
int sensorPin = A0;
int sensorLed = 8;
unsigned long sensorMillis = 0;  // 마지막 측정 시간 저장
const long sensorInterval = 1000;  // 측정 간격(2s)
unsigned long saveMillis = 0;  // 마지막 측정 시간 저장
const long saveInterval = 10000;  // 평균값 저장 간격(30s)

void setup() {
  pinMode(fan1, OUTPUT);
  pinMode(fan2, OUTPUT);
  pinMode(led, OUTPUT);
  pinMode(r, OUTPUT);
  pinMode(g, OUTPUT);
  pinMode(b, OUTPUT);
  pinMode(sensorLed, OUTPUT);
  Serial.begin(9600);
  radio.begin();
}
unsigned long currentMillis;
void loop() {
  currentMillis = millis();
  if(currentMillis - sensorMillis >= sensorInterval)detectSensor();
  if(currentMillis - saveMillis >= saveInterval)saveSensor();
  detectButton();
  detectSignal();
  actByData();
  //Serial.println(currentData.power);
  //Serial.println(currentData.quality);
}
int sensorValue = 0;
int sensingCount = 0;
void detectSensor(){
  sensorMillis = currentMillis;  // 현재 시간 저장
  sensingCount+=1;
  digitalWrite(sensorLed, LOW);  // LED 켜기
  delayMicroseconds(280);
  sensorValue += analogRead(sensorPin);  // 센서값 읽기
  delayMicroseconds(40);
  digitalWrite(sensorLed, HIGH);  // LED 끄기
  Serial.println(analogRead(sensorPin));
}
void saveSensor(){
  saveMillis = currentMillis;
  // 누적 센서값을 평균 먼지 농도(µg/m³)로 변환
  float voltage = (sensorValue/sensingCount) * (5.0 / 1024.0);
  int dustDensity = (int)((0.172*voltage-0.1)*1000);  // 예시 변환 공식, 실제 적용시 보정 필요
  Serial.print("Dust Density: ");
  Serial.print(dustDensity);
  Serial.println(" µg/m3");
  currentData.quality = dustDensity;
  sensorValue = 0;
  sensingCount = 0;
}

bool prv_value;
void detectButton(){
  bool readValue = digitalRead(button);
  if(prv_value!=readValue&&readValue==false)
  {
    currentData.power = !currentData.power;
  }
  prv_value = readValue;
}
void detectSignal(){
  setReceiveMode();
  if (radio.available()) {
    //Serial.println("Data received");
    char request[4];
    radio.read(&request, sizeof(request));
    if(strcmp(request, "get")==0)
    {
      delay(100);
      setSendMode();
      radio.write(&currentData, sizeof(currentData));
    }
    else if(strcmp(request, "put")==0)
    {
      setReceiveMode();
      while(radio.available());
      radio.read(&currentData, sizeof(currentData));
    }
  }
}

void actByData() {
  actPower(currentData.power);
  actLed();
}
void actPower(bool p){
  /*fan*/
  digitalWrite(fan1, p);
  digitalWrite(fan2, p);
  digitalWrite(led, p);
}
void actLed(){
  int red = map(abs(currentData.quality-100),0,50,500,0); 
  int green = map(abs(currentData.quality-50),0,50,500,0); 
  int blue = map(abs(currentData.quality-0),0,50,500,0); 
  analogWrite(r,red);
  analogWrite(g,green);
  analogWrite(b,blue);
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
