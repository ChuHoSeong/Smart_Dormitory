#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <stdio.h>
#include <string.h>

RF24 radio(10, 9); // SPI 버스에 nRF24L01 라디오를 설정하기 위해 CE, CSN를 선언.
const byte address[6] = "00001"; //주소값을 5가지 문자열로 변경할 수 있으며, 송신기와 수신기가 동일한 주소로 해야됨.

void setup() {
  //init Serial
  Serial.begin(9600);
  //init RF
  radio.begin();
  reciveMode();
}

void loop() {
  reciveMode();
  if(radio.available())
  {
    char text[20] = "";
    radio.read(&text, sizeof(text));
    Serial.print("catched : ");
    Serial.println(text);

    sendMode();
    char send[20] = "hello";
    radio.write(&send, sizeof(send));
  }
}

//switch RF to send_mode
void sendMode(){
  radio.openWritingPipe(address); //이전에 설정한 5글자 문자열인 데이터를 보낼 수신의 주소를 설정
  radio.setPALevel(RF24_PA_MIN); //전원공급에 관한 파워레벨을 설정합니다. 모듈 사이가 가까우면 최소로 설정합니다.
  radio.stopListening();
  Serial.println("send_mode");
}
//switch RF to recive_mode
void reciveMode(){
  radio.openReadingPipe(0,address); //이전에 설정한 5글자 문자열인 데이터를 보낼 수신의 주소를 설정
  radio.setPALevel(RF24_PA_MIN); //전원공급에 관한 파워레벨을 설정합니다. 모듈 사이가 가까우면 최소로 설정합니다.
  radio.startListening();
  Serial.println("recive_mode");
}

