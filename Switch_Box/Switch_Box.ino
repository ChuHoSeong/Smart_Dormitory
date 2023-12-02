#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Servo.h>

Servo servo1;
Servo servo2;
Servo servo3;

// Structure to manage servo-related information
struct SERVO {
  Servo* servoPtr;          // Pointer to the servo object
  int buttonPin;            // Pin number for the associated button
  int ledPin;               // Pin number for the associated LED
  bool prv_buttonValue;     // Stores the previous state of the button
};

// Array of SERVO structures for each servo
SERVO servos[3] = {
  {&servo1, 4, A0, false},
  {&servo2, 3, A1, false},
  {&servo3, 2, A2, false}
};

RF24 radio(10, 9);         // RF24 object for wireless communication
const byte address[6] = "00003";

// Structure to hold the power state of each servo
typedef struct {
  bool power[3];
} DATA;

DATA currentData = {{0, 0, 0}};

// Setup function - initializes serial communication, RF module, servos, and default servo positions
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

  servo1.write(90); // Set initial position of servo1
}

// Main loop - checks button states, RF signals, and actuates servos based on current data
void loop() {
  detectButton();
  detectSignal();
  actByData();
}

// Detects button presses and updates the power state in currentData accordingly
void detectButton() {
  for (int i = 0; i < 3; i++) {
    bool readValue = digitalRead(servos[i].buttonPin);
    if (servos[i].prv_buttonValue != readValue && readValue == false) {
      currentData.power[i] = !currentData.power[i];
    }
    servos[i].prv_buttonValue = readValue;
  }
}

// Listens for RF signals and responds to 'get' and 'put' requests
void detectSignal() {
  if (radio.available()) {
    Serial.println("Data received");
    char request[4];
    radio.read(&request, sizeof(request));
    if (strcmp(request, "get") == 0) {
      radio.stopListening();
      delay(100);
      radio.write(&currentData, sizeof(currentData));
      radio.startListening();
    } else if (strcmp(request, "put") == 0) {
      delay(200);
      if (radio.available()) radio.read(&currentData, sizeof(currentData));
    }
  }
}

// Actuates each servo and associated LED based on the current power state
void actByData() {
  for (int i = 0; i < 3; i++) {
    actServo(i, currentData.power[i]);
  }
  Serial.println("\n-----");
}

bool prv_heater_power = false;
// Function to control individual servo's position based on power state
void actServo(int i, bool p) {
  if (i == 0) {
    if (p) servos[i].servoPtr->write(120);
    else servos[i].servoPtr->write(90);
  } else if (i == 1) {
    if (p) servos[i].servoPtr->write(65);
    else servos[i].servoPtr->write(105);
  } else if (i == 2) {
    if (prv_heater_power != p) {
      servos[i].servoPtr->write(50);
      delay(200);
      servos[i].servoPtr->write(80);
    }
    prv_heater_power = p;
  }
  analogWrite(servos[i].ledPin, 150 * (!p)); // Control LED brightness based on power state
}
