#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <DHT.h>

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

#define DHTPIN 4
#define SMOISTURE_PIN 36    

#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

void setup(){
  pinMode(2, OUTPUT);
  Serial.begin(115200);
  dht.begin();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
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

  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 800 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();

    Firebase.RTDB.setInt(&fbdo, "1234/humidity", humidity);
    Firebase.RTDB.setFloat(&fbdo, "1234/temperature", temperature);
    Firebase.RTDB.setFloat(&fbdo, "1234/soilMoisture", soilMoisture);

    if (Firebase.RTDB.getInt(&fbdo, "/test/int")) {
      treshold = fbdo.intData();
    }
  }
  
}


float getTemp(){

  delay(500);
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();


  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return 0;
  }

  return humidity, temperature;
}


int getSoilMoisture(){
  delay(500)
  int value = analogRead(AOUT_PIN);
  
  if(isnan(value)){
    return 0;
  }
  else{
    return value;
  }
}




