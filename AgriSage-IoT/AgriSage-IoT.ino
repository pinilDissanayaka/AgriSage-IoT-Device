#include <Arduino.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <DHT.h>
#include <BH1750.h>

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "Dialog 4G"
#define WIFI_PASSWORD "A5LB331H1G1"

// Insert Firebase project API Key
#define API_KEY "AIzaSyCMsRoYBGZnnttlDbtScuMfhHfqYnF57R0"
// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://agrisage-85205-default-rtdb.asia-southeast1.firebasedatabase.app/" 

#define DHT_PIN 4 // Pin for DHT sensor
#define SMOISTURE_PIN 36 // Pin for Soil moisure sensor
#define WATER_LEVEL_PIN 39 // Pin for water level sensor
#define Ph_PIN 40 // pin for ph sensor
#define RELAY_PIN 12 //pin for water motor relay

// Define RS485 pins for RE and DE to switch between transmit and receive mode
#define RS485_RE 8
#define RS485_DE 7

const byte nitro[] = {0x01, 0x03, 0x00, 0x1e, 0x00, 0x01, 0xe4, 0x0c};
const byte phos[] = {0x01, 0x03, 0x00, 0x1f, 0x00, 0x01, 0xb5, 0xcc};
const byte pota[] = {0x01, 0x03, 0x00, 0x20, 0x00, 0x01, 0x85, 0xc0};

SoftwareSerial modbus(2, 3); // RX, TX

#define DHTTYPE DHT11
DHT dht(DHT_PIN, DHTTYPE);

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

// A byte array to store NPK values
byte values[11];

void setup(){
  pinMode(2, OUTPUT);
  Serial.begin(115200);
  dht.begin();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  modbus.begin(9600);  // Start serial communication with the RS485 module

  // Set RS485 pins as outputs
  pinMode(RS485_RE, OUTPUT);
  pinMode(RS485_DE, OUTPUT);

  // Turn off RS485 receiver and transmitter initially
  digitalWrite(RS485_RE, LOW);
  digitalWrite(RS485_DE, LOW);

  Serial.print("Connecting to Wi-Fi");
  
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    digitalWrite(2, HIGH);
    delay(300);
    digitalWrite(2, LOW);
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  digitalWrite(2, HIGH);

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop(){


  float humidity, temperature = getTemp();
  int soilMoisture = getSoilMoisture();
  byte nitrogen=readValue(nitro);
  byte phosphorus=readValue(phos);
  byte potassium=readValue(pota);
  int waterLevel= getWaterLevel();
  float phLavel=getPhLevel();

  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 800 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();

    Firebase.RTDB.setInt(&fbdo, "1234/humidity", humidity);
    Firebase.RTDB.setFloat(&fbdo, "1234/temperature", temperature);

    Firebase.RTDB.setFloat(&fbdo, "1234/soilMoisture", soilMoisture);

    Firebase.RTDB.setFloat(&fbdo, "1234/phosphorus", phosphorus);
    Firebase.RTDB.setFloat(&fbdo, "1234/nitrogen", nitrogen);
    Firebase.RTDB.setFloat(&fbdo, "1234/potassium", potassium);

    Firebase.RTDB.setInt(&fbdo, "1234/waterLavel", waterLevel);

    Firebase.RTDB.setInt(&fbdo, "1234/phLavel", phLavel);


    if (Firebase.RTDB.getInt(&fbdo, "123/threshold")) {
      int threshold = fbdo.intData();
      waterMotor(threshold, waterLevel);
    }
  }
  
}


float getTemp(){

  delay(500);
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  Serial.println(humidity);
  Serial.println(temperature);


  if (isnan(humidity) || isnan(temperature)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return 0;
  }

  return humidity, temperature;
}


int getSoilMoisture(){
  delay(500);
  int value = analogRead(SMOISTURE_PIN);
  
  if(isnan(value)){
    return 0;
  }
  else{
    return value;
  }
}

byte readValue(const byte* request) {
  // Set RS485 module to transmit mode
  digitalWrite(RS485_RE, HIGH);
  digitalWrite(RS485_DE, HIGH);

  // Send Modbus RTU request to the device
  modbus.write(request, sizeof(request));

  // Set RS485 module to receive mode
  digitalWrite(RS485_RE, LOW);
  digitalWrite(RS485_DE, LOW);

  // Wait for the response to be received
  delay(10);

  // Read the response into the values array
  byte responseLength = modbus.available();
  for (byte i = 0; i < responseLength; i++) {
    values[i] = modbus.read();
  }

  // Return the value from the response
  return values[3] << 8 | values[4];
}

int getWaterLevel(){
  delay(500);
  int value = analogRead(WATER_LEVEL_PIN);
  
  if(isnan(value)){
    return 0;
  }
  else{
    return value;
  }
}

float getPhLevel(){
  delay(500);
  float value=analogRead(Ph_PIN);
  float voltage=value*(3.3/4095.0);
  float phLavel=3.3*voltage;
  return phLavel;
}

void waterMotor(int threshold, int waterLevel){
  if(threshold >= waterLevel){
    digitalWrite(RELAY_PIN, HIGH);
  }
  else{
    digitalWrite(RELAY_PIN, LOW);
  }

}



