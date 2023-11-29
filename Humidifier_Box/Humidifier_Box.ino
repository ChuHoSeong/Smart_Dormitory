#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#include <DHT.h>

RF24 radio(10, 9);
const byte address[6] = "00002";

typedef struct {
  int temp;
  int humidity;
  bool power;
} DATA;



DATA currentData = {0, 0, false};

int button = 4;//임의로 7로 정함
int humidifier = 7;//임의로 6로 정함
DHT dht(8, DHT11);

void setup() {
  pinMode(humidifier, OUTPUT);

  Serial.begin(9600);
  radio.begin();
  dht.begin();
}
void loop() {
  detectSensor();
  detectButton();
  detectSignal();
  actByData();
  Serial.println(currentData.power);
  Serial.println(sizeof(DATA));
}
void detectSensor(){
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  Serial.print(h);
  Serial.print(" / ");
  Serial.println(t);
  currentData.humidity = (int)h;
  currentData.temp = (int)t;
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
    Serial.println("Data received");
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

bool prv_pow = false;
void actPower(bool p){
  //가습기 모듈이 토글버튼식이라 알고리즘이 좀 복잡함,,
  if(prv_pow!=p)
  {
    if(p)
    {
      Serial.println("on!");
      digitalWrite(humidifier, HIGH);
      delay(40);
      digitalWrite(humidifier, LOW);
      delay(50);
    }
    else
    {
      Serial.println("off!");
      digitalWrite(humidifier, HIGH);
      delay(40);
      digitalWrite(humidifier, LOW);
      delay(50);
      digitalWrite(humidifier, HIGH);
      delay(40);
      digitalWrite(humidifier, LOW);
      delay(50);
    }
  }
  prv_pow = p;
}

void actByData() {
  actPower(currentData.power);
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
