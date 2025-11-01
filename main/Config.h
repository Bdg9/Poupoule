#pragma once
#include <Arduino.h>
#include <time.h>

// ====== USER SETTINGS ======
// Put your WiFi here (or leave empty to skip WiFi)
static const char* WIFI_SSID = "YOUR_WIFI_SSID";
static const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";

// Your location (used to compute today's sunset via API)
static constexpr double LATITUDE  = 46.5197;  // Lausanne example
static constexpr double LONGITUDE = 6.6323;

// Timezone string for Europe/Zurich (CET/CEST)
static const char* TZ_EU_ZURICH = "CET-1CEST,M3.5.0/2,M10.5.0/3";

// NTP servers
static const char* NTP1 = "pool.ntp.org";
static const char* NTP2 = "time.google.com";

// ====== EXPORTED “Config” INTERFACE ======
const char* getWifiSsid() { return WIFI_SSID; }
bool hasWifiCredentials() { return (WIFI_SSID && *WIFI_SSID); }

double getLatitude()  { return LATITUDE;  }
double getLongitude() { return LONGITUDE; }
const char* getTZString() { return TZ_EU_ZURICH; }

// ---- Config.cpp functions (declared here, defined in Config.cpp) ----
void connectWifi();
bool syncTimeViaNTP(const char* tzString);
bool fetchTodaySunsetUTC(double lat, double lon, time_t* sunsetUtc);
double getLatitude();
double getLongitude();
const char* getTZString();    // e.g., "CET-1CEST,M3.5.0/2,M10.5.0/3"
const char* getWifiSsid();
bool hasWifiCredentials();