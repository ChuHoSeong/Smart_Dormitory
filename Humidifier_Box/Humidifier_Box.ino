#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#include <DHT.h>

RF24 radio(10, 9);       // Create an RF24 object for wireless communication
const byte address[6] = "00002";

// Structure to store temperature, humidity, and power status
typedef struct {
  int temp;
  int humidity;
  bool power;
} DATA;

DATA currentData = {0, 0, false};

int button = 4;          // Button pin for toggling power
int humidifier = 7;      // Humidifier control pin
DHT dht(8, DHT11);       // DHT sensor object for temperature and humidity

// Setup function - initializes pins, serial communication, RF module, and DHT sensor
void setup() {
  pinMode(humidifier, OUTPUT);
  digitalWrite(humidifier, HIGH);

  Serial.begin(9600);

  // Initialize RF communication
  radio.begin();
  radio.setPALevel(RF24_PA_HIGH);
  radio.openWritingPipe(address);
  radio.openReadingPipe(0, address);
  radio.startListening();

  // Initialize DHT sensor
  dht.begin();
}

// Main loop function - repeatedly checks sensor data, button status, and RF signals
void loop() {
  detectSensor();
  detectButton();
  detectSignal();
  actByData();
  Serial.println(currentData.power);
  Serial.println(sizeof(DATA));
}

// Reads sensor data and updates the currentData structure
void detectSensor() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  Serial.print(h);
  Serial.print(" / ");
  Serial.println(t);
  currentData.humidity = (int)h;
  currentData.temp = (int)t;
}

bool prv_value;
// Checks the button state and toggles the power in currentData
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

bool prv_pow = false;
// Actuates the humidifier based on the current power state
void actPower(bool p) {
  // The humidifier module is a toggle button, so the algorithm is a bit complex
  if (prv_pow != p) {
    if (p) {
      Serial.println("on!");
      digitalWrite(humidifier, LOW);
      delay(5);
      digitalWrite(humidifier, HIGH);
    } else {
      Serial.println("off!");
      digitalWrite(humidifier, LOW);
      delay(5);
      digitalWrite(humidifier, HIGH);
      delay(5);
      digitalWrite(humidifier, LOW);
      delay(5);
      digitalWrite(humidifier, HIGH);
    }
  }
  prv_pow = p;
}

// Calls actPower function and prints a separator for readability
void actByData() {
  actPower(currentData.power);
  Serial.println("-----");
}
