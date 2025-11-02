#ifndef CONFIG_H
#define CONFIG_H

// ====== HARDWARE SETTINGS ======
// GPIOs (change if needed)
static constexpr int GREEN_LED_PIN = 2;
static constexpr int RED_LED_PIN   = 4;
static constexpr int BUZZER_PIN    = 15;
static constexpr int BUTTON_PIN    = 13;

// ====== DEFAULT SCHEDULE ======
// All times are in LOCAL time (will be adjusted based on timezone & DST)

int MORNINGREDON_H    = 8;    // 08:00
int MORNINGREDON_M    = 45;
int MORNINGALARM_H    = 9;    // 09:00
int MORNINGALARM_M    = 0;
int NAPSTART_H        = 11;   // 11:00
int NAPSTART_M        = 0;
int NAPEND_H          = 15;   // 15:00
int NAPEND_M        = 0;

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

#endif // CONFIG_H
