/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-date-time-ntp-client-server-arduino/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include "time.h"
#include <Arduino.h>

#define SUNSET_URL "https://api.sunrise-sunset.org"


String sunsetMessage = "/json?lat=46.597149&lng=6.756825&date=today&tzid=Europe/Zurich";

const char* ssid     = "Cultivarium";
const char* password = "Bienvenue";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 1;
const int   daylightOffset_sec = 3600;

void setup(){
  Serial.begin(115200);

  // Connect to Wi-Fi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  
  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();

  //disconnect WiFi as it's no longer needed
  // WiFi.disconnect(true);
  // WiFi.mode(WIFI_OFF);
}

void loop(){
  delay(5000);
  printLocalTime();
  printSunSetTime(&sunsetMessage);

}

void printLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  Serial.print("Day of week: ");
  Serial.println(&timeinfo, "%A");
  Serial.print("Month: ");
  Serial.println(&timeinfo, "%B");
  Serial.print("Day of Month: ");
  Serial.println(&timeinfo, "%d");
  Serial.print("Year: ");
  Serial.println(&timeinfo, "%Y");
  Serial.print("Hour: ");
  Serial.println(&timeinfo, "%H");
  Serial.print("Hour (12 hour format): ");
  Serial.println(&timeinfo, "%I");
  Serial.print("Minute: ");
  Serial.println(&timeinfo, "%M");
  Serial.print("Second: ");
  Serial.println(&timeinfo, "%S");

  Serial.println("Time variables");
  char timeHour[3];
  strftime(timeHour,3, "%H", &timeinfo);
  Serial.println(timeHour);
  char timeWeekDay[10];
  strftime(timeWeekDay,10, "%A", &timeinfo);
  Serial.println(timeWeekDay);
  Serial.println();
}

void printSunSetTime(String *sunsetMsg){
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected");
    return;
  }

  HTTPClient http;
  String url = String(SUNSET_URL) + *sunsetMsg; // build full URL: https://api.sunrise-sunset.org/json?...
  Serial.println("Request URL: " + url);

  if (!http.begin(url)) {   // begin with full URL (no WiFiClientSecure used)
    Serial.println("HTTP begin failed");
    return;
  }

  // For this API use GET (no body). No need for addHeader here.
  int response_code = http.GET();

  if (response_code > 0) {
    Serial.print("HTTP code: ");
    Serial.println(response_code);
    if (response_code == HTTP_CODE_OK) {
      String response_body = http.getString();
      Serial.println("Server reply:");
      Serial.println(response_body);
    }
  } else {
    Serial.print("Request failed, error: ");
    Serial.println(response_code); // -1 indicates connection/TLS issue
  }

  http.end();
}