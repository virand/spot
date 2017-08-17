// ВАЖНО! У каждого устройства свой уникальный ID
#define DEVICE_ID 1
#define URL_FOR_REQUESTS "http://176.53.223.177:10080/spot/sendValue.php?deviceId="
#define WIFI_SSID "VIRAND.RU"
#define WIFI_PASSWORD "2329292hex"

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>

#define USE_SERIAL Serial

ESP8266WiFiMulti WiFiMulti;

unsigned int triggerValue=0;

unsigned long Time_Echo_us = 0;
unsigned long Len_mm_X100 = 0;
unsigned long Len_Integer = 0; 
unsigned int Len_Fraction = 0;
boolean connectionEstablished=false;
boolean ultrasonicConnected=false;
//Len_mm_X100 = length*100

unsigned int EchoPin = 16;
unsigned int TrigPin = 5;
unsigned int PowerPin = 15;
unsigned int NetworkPin = 4;
unsigned int CarIsPresentPin = 0;


void setup(){
  Serial.begin(9600);
  pinMode(EchoPin, INPUT);
  pinMode(TrigPin, OUTPUT);
  pinMode(PowerPin, OUTPUT);
  pinMode(NetworkPin, OUTPUT);
  pinMode(CarIsPresentPin, OUTPUT);

  digitalWrite(PowerPin, HIGH);


  USE_SERIAL.println();
  USE_SERIAL.println();
  USE_SERIAL.println();

  for(uint8_t t = 4; t > 0; t--) {
        USE_SERIAL.printf("[SETUP] WAIT %d...\n", t);
        USE_SERIAL.flush();
        delay(1000);
  }

  WiFiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
}




// Возвращает расстояние между датчиком и поверхностью
unsigned int getDistance(){
  digitalWrite(TrigPin, HIGH);
  delayMicroseconds(50);
  digitalWrite(TrigPin, LOW);
  Time_Echo_us = pulseIn(EchoPin, HIGH);
  unsigned int distance=0;

if((Time_Echo_us < 60000) && (Time_Echo_us > 1)){
    Len_mm_X100 = (Time_Echo_us*34)/2;
    Len_Integer = Len_mm_X100/100;
    Len_Fraction = Len_mm_X100%100;
    Serial.print("Present Length is: ");
    Serial.print(Len_Integer, DEC);
    distance = Len_Integer;
    Serial.print(".");
    if(Len_Fraction < 10)
      Serial.print("0");
    Serial.print(Len_Fraction, DEC);
    Serial.println("mm");
    ultrasonicConnected=true;
  }
  else
  {
    ultrasonicConnected=false;
  }
  

  return distance;
}




// Отправить результаты на сервер, возврашает triggerValue для данного устройства
unsigned int makeRequest(unsigned int distance){

  unsigned int triggerValue=0;
  
   // wait for WiFi connection
    if((WiFiMulti.run() == WL_CONNECTED)) {

        HTTPClient http;

        USE_SERIAL.print("[HTTP] begin...\n");
        // configure traged server and url
        //http.begin("https://192.168.1.12/test.html", "7a 9c f4 db 40 d3 62 5a 6e 21 bc 5c cc 66 c8 3e a1 45 59 38"); //HTTPS
        char url[120];
        strcpy(url, URL_FOR_REQUESTS);
        char str_device_id[20];
        sprintf(str_device_id, "%d", DEVICE_ID);
        strcat(url, str_device_id);
        strcat(url, "&value=");
        char str_distance[50];
        sprintf(str_distance, "%d", distance);
        strcat(url, str_distance);
        Serial.print("URL: ");
        Serial.println(url);
        http.begin(url); //HTTP

        USE_SERIAL.print("[HTTP] GET...\n");
        // start connection and send HTTP header
        int httpCode = http.GET();

        // httpCode will be negative on error
        if(httpCode > 0) {
            // HTTP header has been send and Server response header has been handled
            USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);

            // file found at server
            if(httpCode == HTTP_CODE_OK) {
                String payload = http.getString();
                USE_SERIAL.print("[PAGE_CONTENT]: ");
                USE_SERIAL.println(payload);
                triggerValue=payload.toInt();
            }
            connectionEstablished=true;
        } else {
            USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
            connectionEstablished=false;
        }

        http.end();
    }

    return triggerValue;
}



void loop(){
  unsigned int distance = 0;
  distance = getDistance();
  triggerValue=makeRequest(distance);

  if(connectionEstablished)
  {
      digitalWrite(NetworkPin, HIGH);
      if (distance<triggerValue && ultrasonicConnected)
      {
        digitalWrite(CarIsPresentPin, HIGH);
      }
      else{
        digitalWrite(CarIsPresentPin, LOW);
      }
  }
  else
  {
      digitalWrite(NetworkPin, LOW);
      digitalWrite(CarIsPresentPin, LOW);
  }


  


  delay(1000);
}
