// Include all the libraries required
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include "DHTesp.h"
#ifdef ESP32
#pragma message(THIS EXAMPLE IS FOR ESP8266 ONLY!)
#error Select ESP8266 board.
#endif

#define GPIO_DHT_SENSOR 15 // D8. Feather HUZZAH ESP8266 note: use pins 3, 4, 5, 12, 13 or 14 --
#define GPIO_SR04_TRIG 2 // D4
#define GPIO_SR04_ECHO 0 // D3
#define GPIO_FLAME_SENSOR 13// D7

// Create a struct with all the properties
struct received_data{
  bool lamp_state = false;
  bool extenguisher_state = false;
  float x_position = 0.0;
  float y_position = 0.0; 
  int pos_of_servo_cam = 0;
};

long millis_temp = 0;

// Credentials
const char* ssid = "asv's den";
const char* password = "8i654321";

// Setup the server
WiFiServer server(1337);
DHTesp dht;
struct received_data recvd;

void setup(void) {
  Serial.begin(115200);
  pinMode(GPIO_SR04_TRIG, OUTPUT); // Sets the trigPin as an Output
  pinMode(GPIO_SR04_ECHO, INPUT); // Sets the echoPin as an Input
  pinMode(GPIO_FLAME_SENSOR, INPUT); // Sets the flame sensor's OUT pin as an Input
  Serial.println("Setting AP (Access Point)...");
  // Connect to the server
  WiFi.begin(ssid, password);
  while(WiFi.status()!=WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  // Start TCP server
  server.begin();
  Serial.println("Server started");
  // Obtain the Local IP
  Serial.println(WiFi.localIP());
  dht.setup(GPIO_DHT_SENSOR, DHTesp::DHT11);
}

StaticJsonDocument<200> doc;

void loop(){
  WiFiClient client = server.available();
  if(client){
    Serial.println("Client connected...");
    while(client.connected()){
      char datas[40]={0};int i = 0;
      bool flag = false;
        if(client.available()>0){
          Serial.println("@");
          while(client.available()){
            if(client.available()){
            char temp = client.read();
            if(temp=='\n'){
              break;
            }
            datas[i++] = temp;
            }
            flag = true;
          }
          
          String json_data = String(datas);
          Serial.println(json_data);
          DeserializationError error = deserializeJson(doc, json_data);

          // Test if parsing succeeds.
          if (error) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
            return;
          }
          
          recvd.lamp_state = doc["l"];
          recvd.extenguisher_state = doc["w"];
          recvd.x_position = doc["x"];
          recvd.y_position = doc["y"];
          recvd.pos_of_servo_cam = doc["s"];
          
          flag = true;
          
        }
        Serial.println(">");
        
        // Clears the TRIGGER pin
        digitalWrite(GPIO_SR04_TRIG, LOW);
        delayMicroseconds(2);
        // Sets the TRIGGER pin on HIGH state for 10 micro seconds
        digitalWrite(GPIO_SR04_TRIG, HIGH);
        delayMicroseconds(10);
        digitalWrite(GPIO_SR04_TRIG, LOW);
        // Reads the ECHOPIN, returns the sound wave travel time in microseconds
        float duration = pulseIn(GPIO_SR04_ECHO, HIGH);
        // Calculating the distance
        int distance= duration*0.034/2;
        float temperature = dht.getTemperature();
        float humidity = dht.getHumidity();
        if(digitalRead(GPIO_FLAME_SENSOR)==1){
          doc["fir"] = 0;
        }else{
        doc["fir"] = 1;
        }
        if(millis()-millis_temp > 800){
          Serial.println(".");
        if((String(humidity)!="nan")&&(String(temperature)!="nan")){
          doc["tem"] = temperature;
          doc["hum"] = humidity;
        }
        doc["obs"] = distance;
        String temmm;
        serializeJson(doc, temmm);
        client.println(temmm);
        millis_temp = millis();
        }
        
    }
    Serial.println("Client disconnected:<");
    client.stop();
  }
  
}
