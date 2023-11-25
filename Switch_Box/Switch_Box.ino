#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Servo.h>

Servo servo1;
Servo servo2;
Servo servo3;

Servo* servos[3] = {&servo1, &servo2, &servo3};
RF24 radio(10, 9);
const byte address[6] = "00003";

typedef struct {
  bool power[3];
  bool changed;
} SWI;


SWI switchControl;
SWI recivedData;
int switchPins[3] = {2, 3, 4};

void setup() {
  Serial.begin(9600);
  radio.begin();

  servo1.attach(7);
  servo2.attach(6);
  servo3.attach(5);

  for (int i = 0; i < 3; i++) {
    pinMode(switchPins[i], INPUT_PULLUP);
  }

  recivedData = {{0, 0, 0}, 0};
  switchControl = {{0, 0, 0}, 0};

  servo1.write(100);
}

void loop() {
  for (int i = 0; i < 3; i++) {
    recivedData.power[i] = digitalRead(switchPins[i]) == LOW; // Assuming LOW when button is pressed
  }

  processSwitchData();

  switchControl = recivedData;
  setReceiveMode();

  if (radio.available()) {
    Serial.println("Data received");
    char request[4];
    radio.read(&request, sizeof(request));
    if (strcmp(request, "get") == 0) {
      setSendMode();
      radio.write(&switchControl, sizeof(SWI));
    } else if (strcmp(request, "put") == 0) {
      setReceiveMode();
      radio.read(&recivedData, sizeof(SWI));
    }
  }
}

void processSwitchData() {
  for (int i = 0; i < 3; i++) {
    if (switchControl.power[i] == recivedData.power[i]) continue;
    else if (switchControl.power[i] < recivedData.power[i]) {
      // i번 스위치 on
      servos[i]->write(90);
      delay(300);
      // 버튼이 눌렸을 때, 서보 동작을 멈추도록 추가
      if (!recivedData.power[i]) {
        servos[i]->detach(); // 서보를 일시적으로 중지
        delay(300); // 중지한 상태를 유지할 시간 (선택적)
        servos[i]->attach(i == 0 ? 7 : (i == 1 ? 6 : 5)); // 다시 서보를 연결
      }
      servos[i]->write(180);
    }
    else if (switchControl.power[i] > recivedData.power[i]) {
      // i번 스위치 off
      // 서보를 끄는 동작 추가 (필요에 따라 처리)
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
