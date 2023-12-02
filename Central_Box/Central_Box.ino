// Board related libraries
#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
// Firebase related libraries
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
// WiFi communication settings
#define WIFI_SSID "452"
#define WIFI_PASSWORD "00000452"
#define API_KEY "API key here"
#define DATABASE_URL "https://your-database-url.firebaseio.com/" 
// nRF24 module related libraries
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
// LCD library
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

// Define addresses for RF communication devices
const byte AIR_CLEANER_ADDRESS[6] = "00001"; // Air cleaner address
const byte HUMIDIFIER_ADDRESS[6] = "00002"; // Humidifier address
const byte SWITCH_ADDRESS[6] = "00003"; // Switch address

// Create RF24 object (specify CE, CSN pin numbers)
RF24 radio(2, 0); 

// Define data structures
typedef struct __attribute__((packed)){
  int16_t quality;
  bool power;
} AirCleanerData; // Air cleaner data structure
typedef struct __attribute__((packed)){
  int16_t temp;
  int16_t humidity;
  bool power;
} HumidifierData; // Humidifier data structure
typedef struct {
  bool power[3];
} SwitchData; // Switch data structure

// Define structures containing data for each device
struct AIR{
  AirCleanerData center;
  AirCleanerData device;
  AirCleanerData cloude;
  bool connected = false;
}airCleaner;
struct HUM{
  HumidifierData center;
  HumidifierData device;
  HumidifierData cloude;
  bool connected = false;
}humidifier;
struct SWI{
  SwitchData center;
  SwitchData device;
  SwitchData cloude;
  bool connected = false;
}switchControl;

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool signupOK = false;

// Define LCD object
LiquidCrystal_I2C lcd(0x27, 16, 2); 

// Initial setup function
void setup() {
  Serial.begin(9600); // Start serial communication

  // Start Wi-Fi communication
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else Serial.printf("%s\n", config.signer.signupError.message.c_str());
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  radio.begin(); // Start RF communication
  radio.setPALevel(RF24_PA_HIGH); // Set communication strength

  Wire.begin(D2, D1); // Start I2C communication (SDA, SCL)
  lcd.init(); // Initialize LCD
  lcd.backlight(); // Turn on backlight
  lcd.setCursor(0, 0);
  lcd.print("??");
  lcd.write(byte(0xDF));
  lcd.print("C ??% ??ug/m3");
  lcd.setCursor(0, 1);
  lcd.print("S1:?  S2:?  S3:?");

  // Perform initial setup for each device
  init(AIR_CLEANER_ADDRESS, airCleaner, "Air Cleaner");
  init(HUMIDIFIER_ADDRESS, humidifier, "Humidifier");
  init(SWITCH_ADDRESS, switchControl, "Switch");

  Serial.println("--init completed");
}

// Template function for initializing device data
template<typename T>
void init(const byte address[], T &deviceData, const char* deviceName) {
  radio.openWritingPipe(address); // Set transmission address
  radio.openReadingPipe(0, address); // Set reception address

  // Retrieve data from the device.
  get(address, &deviceData.device, sizeof(deviceData.device), deviceName);
  // Initialize 'center' and 'cloude' data with the current device data (status).
  deviceData.cloude = deviceData.device;
  deviceData.center = deviceData.device;

  // Upload deviceData.cloude value to the server
  toFirebase(deviceData.cloude);
}

// Main loop function
void loop() {
  // Update the status information of air cleaner, humidifier, and switch using template function.
  syncData(AIR_CLEANER_ADDRESS, airCleaner, "Air Cleaner");
  syncData(HUMIDIFIER_ADDRESS, humidifier, "Humidifier");
  syncData(SWITCH_ADDRESS, switchControl, "Switch");
  printLcd();
  Serial.println("--------------------");
}

// Template function for updating each device's data
template<typename T>
void syncData(const byte address[], T &deviceData, const char* deviceName) {
  radio.openWritingPipe(address); // Set transmission address
  radio.openReadingPipe(0, address); // Set reception address

  // Retrieve status data from the device.
  deviceData.connected = get(address, &deviceData.device, sizeof(deviceData.device), deviceName);
  // Retrieve data from the server.
  fromFirebase(deviceData.cloude, deviceName);

  if(checkChangePower(deviceData.center, deviceData.cloude)) { // No changes on the server
    Serial.println("change detected cloude");
    deviceData.center = deviceData.cloude;
    deviceData.device = deviceData.cloude;
    put(address, &deviceData.device, sizeof(deviceData.device), deviceName);
  }
  else
  {
    // Continuous synchronization
    Serial.println("none");
    deviceData.center = deviceData.device;
    deviceData.cloude = deviceData.device;
    // Upload deviceData.cloude value to the server
    toFirebase(deviceData.cloude);
  }
}

// Function to compare power states of data1 and data2
template<typename T> bool checkChangePower(T &data1, T &data2){
  return (data1.power != data2.power);
}
template<> bool checkChangePower<SwitchData>(SwitchData &data1, SwitchData &data2){
  for(int i = 0;i<3;i++)
  {
    if(data1.power[i]!=data2.power[i])return true;
  }
  return false;
}

// Function to receive data from a specific address device
bool get(const byte address[], void* data, size_t dataSize, const char* deviceName){
  radio.stopListening();
  char request[4] = "get"; // 'get' request message
  radio.write(&request, sizeof(request)); // Send request message
  radio.startListening();
  delay(200);
  if(radio.available()){
    radio.read(data, dataSize); // Receive data
    Serial.print("realtime_size = ");
    Serial.println(dataSize);
    return true;
  }
  else 
  {
    Serial.println(String("*") + deviceName + " system is turned off"); // Print system off message
    return false;
  } 
}

// Function to send data to a specific address device
void put(const byte address[], void* data, size_t dataSize, const char* deviceName){
  radio.stopListening();// Set to transmission mode
  char request[4] = "put";
  radio.write(&request, sizeof(request)); // Send request message
  delay(100); // Delay for safety
  radio.write(data, dataSize); // Send data
}

// Function to display status information on the LCD
void printLcd(){
  char buffer[4]; 

  if(humidifier.connected)
  {
    lcd.setCursor(0, 0);
    sprintf(buffer, "%2d", humidifier.center.temp);
    lcd.print(buffer);
    lcd.setCursor(4, 0);
    sprintf(buffer, "%3d", humidifier.center.humidity);
    lcd.print(buffer);
  }
  else
  {
    lcd.setCursor(0, 0);
    lcd.print("??");
    lcd.setCursor(5, 0);
    lcd.print("??");
  }
  if(airCleaner.connected)
  {
    lcd.setCursor(8, 0);
    sprintf(buffer, "%3d", airCleaner.center.quality);
    lcd.print(buffer);
  }
  else
  {
    lcd.setCursor(8, 0);
    lcd.print(" ??");
  }
  if(switchControl.connected)
  {
    lcd.setCursor(3, 1);
    lcd.print(switchControl.center.power[0] ? "O" : "X");
    lcd.setCursor(9, 1);
    lcd.print(switchControl.center.power[1] ? "O" : "X");
    lcd.setCursor(15, 1);
    lcd.print(switchControl.center.power[2] ? "O" : "X");
  }
  else
  {
    lcd.setCursor(3, 1);
    lcd.print("?");
    lcd.setCursor(9, 1);
    lcd.print("?");
    lcd.setCursor(15, 1);
    lcd.print("?");
  }
}


// Function to print data to the serial monitor (FOR DEBUGING)
template<typename T>
void printDeviceData(T &data){
  Serial.println("print : Unknown Device Type");
}
template<>
void printDeviceData<AirCleanerData>(AirCleanerData &data) {
  Serial.print("* Power: "); Serial.print(data.power);
  Serial.print(", Quality: "); Serial.println(data.quality);
}
template<>
void printDeviceData<HumidifierData>(HumidifierData &data) {
  Serial.print("* Power: "); Serial.print(data.power);
  Serial.print(", Temp: "); Serial.print(data.temp);
  Serial.print(", Humidity: "); Serial.println(data.humidity);
}
template<>
void printDeviceData<SwitchData>(SwitchData &data) {
  Serial.print("* Switch States: ");
  for (int i = 0; i < 3; i++) {
    Serial.print(data.power[i]); Serial.print(" ");
  }
  Serial.println();
}

/* functions for communicate with firebase */

template<typename T>
bool fromFirebase(T &deviceData, const char *deviceName) {
  Serial.println("firebase : Unknown Device Type");
  return false;
}
template<>
bool fromFirebase<AirCleanerData>(AirCleanerData &deviceData, const char *deviceName) {
  // If Firebase is not ready to be used, return false
  if (!Firebase.ready() || !signupOK) {
    Serial.println(fbdo.errorReason());
    return false;
  }
  // Access the object in the DB with the device name
  // If access fails, print the error code and return false
  if (Firebase.RTDB.getJSON(&fbdo, deviceName) == false) {
    Serial.println(fbdo.errorReason());
    return false;
  }
  // Process of storing the data saved as json
  FirebaseJson json = fbdo.jsonObject();
  FirebaseJsonData data;
  json.get(data, "power");
  deviceData.power = data.to<bool>();
  json.get(data, "quality");
  deviceData.quality = data.to<int>();
  return true;
}
template<>
bool fromFirebase<HumidifierData>(HumidifierData &deviceData, const char *deviceName) {
  // If Firebase is not ready to be used, return false
  if (!Firebase.ready() || !signupOK) {
    Serial.println(fbdo.errorReason());
    return false;
  }
  // Access the object in the DB with the device name
  // If access fails, print the error code and return false
  if (Firebase.RTDB.getJSON(&fbdo, deviceName) == false) {
    Serial.println(fbdo.errorReason());
    return false;
  }
  // Process of storing the data saved as json
  FirebaseJson json = fbdo.jsonObject();
  FirebaseJsonData data;
  json.get(data, "power");
  deviceData.power = data.to<bool>();
  json.get(data, "humidity");
  deviceData.humidity = data.to<int>();
  json.get(data, "temp");
  deviceData.temp = data.to<int>();
  return true;
}
template<>
bool fromFirebase<SwitchData>(SwitchData &deviceData, const char *deviceName) {
  // If Firebase is not ready to be used, return false
  if (!Firebase.ready() || !signupOK) {
    Serial.println(fbdo.errorReason());
    return false;
  }
  // Access the object in the DB with the device name
  // If access fails, print the error code and return false
  if (Firebase.RTDB.getJSON(&fbdo, deviceName) == false) {
    Serial.println(fbdo.errorReason());
    return false;
  }
  // Process of storing the data saved as json
  FirebaseJson json = fbdo.jsonObject();
  FirebaseJsonData data;
  FirebaseJsonArray ary;
  json.get(data, "power");
  data.get<FirebaseJsonArray>(ary);
  for (size_t i=0 ; i < ary.size() ; i++) {
    ary.get(data, i);
    deviceData.power[i] = data.to<bool>();
  }
  return true;
}

template<typename T>
bool toFirebase(T &deviceData) {
  Serial.println("Unknown Device Type");
  return false;
}
template<>
bool toFirebase<AirCleanerData>(AirCleanerData &deviceData) {
  // If Firebase is not ready to be used, return false
  if (!Firebase.ready() || !signupOK) {
    Serial.println(fbdo.errorReason());
    return false;
  }
  // Process of converting data into json
  FirebaseJson json;
  json.set("/Air Cleaner/power", deviceData.power);
  json.set("/Air Cleaner/quality", deviceData.quality);
  if (Firebase.RTDB.updateNode(&fbdo, "/", &json) == false) {
    Serial.println(fbdo.errorReason());
    return false;    
  }
  return true;
}
template<>
bool toFirebase<HumidifierData>(HumidifierData &deviceData) {
  // If Firebase is not ready to be used, return false
  if (!Firebase.ready() || !signupOK) {
    Serial.println(fbdo.errorReason());
    return false;
  }
  // Process of converting data into json
  FirebaseJson json;
  json.set("/Humidifier/power", deviceData.power);
  json.set("/Humidifier/humidity", deviceData.humidity);
  json.set("/Humidifier/temp", deviceData.temp);
  if (Firebase.RTDB.updateNode(&fbdo, "/", &json) == false) {
    Serial.println(fbdo.errorReason());
    return false;    
  }
  return true;
}
template<>
bool toFirebase<SwitchData>(SwitchData &deviceData) {
  // If Firebase is not ready to be used, return false
  if (!Firebase.ready() || !signupOK) {
    Serial.println(fbdo.errorReason());
    return false;
  }
  // Process of converting data into json
  FirebaseJson json;
  for (int i=0 ; i < 3 ; i++)
    json.set("/Switch/power/" + String(i), deviceData.power[i]);
  if (Firebase.RTDB.updateNode(&fbdo, "/", &json) == false) {
    Serial.println(fbdo.errorReason());
    return false;    
  }
  return true;
}
