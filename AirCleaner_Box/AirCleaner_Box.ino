#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <math.h>

RF24 radio(10, 9); // RF24 object for wireless communication
const byte address[6] = "00001";

// Data structure to store air quality and power status
typedef struct {
  int quality;
  bool power;
} DATA;

DATA currentData;

// Pin assignments
int button = 4; // Button pin for toggling power
int fan1 = 7; // First fan control pin
int fan2 = 6; // Second fan control pin
int led = 5; // LED control pin
int r = A3; // Red LED pin
int g = A4; // Green LED pin
int b = A5; // Blue LED pin
int sensorPin = A0; // Air quality sensor pin
int sensorLed = 8; // Sensor LED pin
unsigned long sensorMillis = 0; // Last sensor reading time
const long sensorInterval = 1000; // Sensor reading interval (1s)
unsigned long saveMillis = 0; // Last save time
const long saveInterval = 10000; // Data saving interval (10s)

// Setup function - initializes pins, serial communication, and RF module
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
  radio.setPALevel(RF24_PA_HIGH);
  radio.openWritingPipe(address);
  radio.openReadingPipe(0, address);
  radio.startListening();
}

unsigned long currentMillis;
// Main loop - checks sensor, button status, RF signals, and actuates devices
void loop() {
  currentMillis = millis();
  if (currentMillis - sensorMillis >= sensorInterval) detectSensor();
  if (currentMillis - saveMillis >= saveInterval) saveSensor();
  detectButton();
  detectSignal();
  actByData();
}

int sensorValue = 0;
int sensingCount = 0;
// Reads air quality sensor data at regular intervals
void detectSensor() {
  sensorMillis = currentMillis; // Update last sensor reading time
  sensingCount += 1;
  digitalWrite(sensorLed, LOW); // Turn on sensor LED
  delayMicroseconds(280);
  sensorValue += analogRead(sensorPin); // Read sensor value
  delayMicroseconds(40);
  digitalWrite(sensorLed, HIGH); // Turn off sensor LED
  Serial.println(analogRead(sensorPin));
}

// Saves the average sensor data at regular intervals
void saveSensor() {
  saveMillis = currentMillis;
  // Convert accumulated sensor readings to average dust density
  float voltage = (sensorValue / sensingCount) * (5.0 / 1024.0);
  int dustDensity = (int)((0.172 * voltage - 0.1) * 1000); // Example conversion formula, calibration needed for actual application
  Serial.print("Dust Density: ");
  Serial.print(dustDensity);
  Serial.println(" µg/m3");
  currentData.quality = dustDensity;
  sensorValue = 0;
  sensingCount = 0;
}

bool prv_value;
// Detects button presses and toggles the power state in currentData
void detectButton() {
  bool readValue = digitalRead(button);
  if (prv_value != readValue && readValue == false) {
    currentData.power = !currentData.power;
  }
  prv_value = readValue;
}

// Listens for RF signals and responds to 'get' and 'put' requests
void detectSignal() {
  if (radio.available()) {
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

// Actuates fans, LED, and colored LEDs based on current data
void actByData() {
  actPower(currentData.power);
  actLed();
}

// Controls power to the fans and main LED
void actPower(bool p) {
  // Due to current shortage issue, two pins are used to supply power to the fan
  digitalWrite(fan1, p);
  digitalWrite(fan2, p);
  digitalWrite(led, p);
}

// Adjusts the color of RGB LEDs based on air quality
void actLed() {
  // Maps air quality values to RGB color intensity
  int red = map(abs(currentData.quality - 100), 0, 50, 500, 0); 
  int green = map(abs(currentData.quality - 50), 0, 50, 500, 0); 
  int blue = map(abs(currentData.quality - 0), 0, 50, 500, 0); 
  analogWrite(r, red);
  analogWrite(g, green);
  analogWrite(b, blue);
}
