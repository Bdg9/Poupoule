#include "Config.h"
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

void connectWifi() {
  if (!hasWifiCredentials()) return;
  if (WiFi.status() == WL_CONNECTED) return;

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("[WiFi] Connecting to %s", WIFI_SSID);

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[WiFi] Connected. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("[WiFi] Failed to connect.");
  }
}

bool syncTimeViaNTP(const char* tzString) {
  // Set TZ for localtime()
  if (tzString && *tzString) {
    setenv("TZ", tzString, 1);
    tzset();
  }

  configTime(0, 0, NTP1, NTP2, nullptr);

  // Wait up to ~10s
  for (int i = 0; i < 100; ++i) {
    time_t now = time(nullptr);
    if (now > 8 * 3600UL) {
      Serial.print("[Time] Synced: ");
      Serial.print(ctime(&now));
      return true;
    }
    delay(100);
  }
  return false;
}

// Fetch today's sunset via Sunrise-Sunset API (UTC, ISO8601)
// Docs: https://sunrise-sunset.org/api
bool fetchTodaySunsetUTC(double lat, double lon, time_t* sunsetUtc) {
  if (!sunsetUtc) return false;

  if (WiFi.status() != WL_CONNECTED) {
    connectWifi();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[Sunset] No WiFi, cannot fetch.");
      return false;
    }
  }

  // "date=today&formatted=0" returns ISO 8601 in UTC
  char url[256];
  snprintf(url, sizeof(url),
           "https://api.sunrise-sunset.org/json?lat=%f&lng=%f&date=today&formatted=0",
           lat, lon);

  HTTPClient http;
  http.begin(url);
  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("[Sunset] HTTP error: %d\n", code);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  StaticJsonDocument<2048> doc;
  auto err = deserializeJson(doc, payload);
  if (err) {
    Serial.print("[Sunset] JSON parse error: ");
    Serial.println(err.f_str());
    return false;
  }

  const char* status = doc["status"] | "";
  if (strcmp(status, "OK") != 0) {
    Serial.printf("[Sunset] API status: %s\n", status);
    return false;
  }

  const char* sunsetIso = doc["results"]["sunset"] | nullptr;
  if (!sunsetIso) {
    Serial.println("[Sunset] No 'sunset' field.");
    return false;
  }

  // Parse ISO8601 UTC (YYYY-MM-DDTHH:MM:SS+00:00)
  // We'll do a minimal parse assuming +00:00 and no leap seconds.
  int Y,M,D,h,m,s;
  if (sscanf(sunsetIso, "%d-%d-%dT%d:%d:%d", &Y,&M,&D,&h,&m,&s) != 6) {
    Serial.printf("[Sunset] Failed to parse: %s\n", sunsetIso);
    return false;
  }

  struct tm tmUTC = {};
  tmUTC.tm_year = Y - 1900;
  tmUTC.tm_mon  = M - 1;
  tmUTC.tm_mday = D;
  tmUTC.tm_hour = h;
  tmUTC.tm_min  = m;
  tmUTC.tm_sec  = s;
  // timegm converts tm (UTC) to epoch; not standard everywhere, but on ESP32 it's available as timegm.
  time_t t = timegm(&tmUTC);
  *sunsetUtc = t;

  Serial.print("[Sunset] Today sunset (UTC): ");
  Serial.print(ctime(sunsetUtc));
  return true;
}
