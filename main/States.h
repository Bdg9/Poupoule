#pragma once
#include <Arduino.h>
#include <time.h>

// ---- States ----
enum State {
  SLEEP,
  ALLGOOD,         // Green ON, Red OFF
  CLOSEDOOR,       // Red ON, Green OFF (waiting for human action)
  NOBODYCLOSEDIT,  // Buzzer alarming until button press
  ERRROR
};

// ---- State Machine ----
class StateMachine {
public:
  void begin();
  void update();

private:
  // GPIOs (change if needed)
  static constexpr int GREEN_LED_PIN = 2;
  static constexpr int RED_LED_PIN   = 4;
  static constexpr int BUZZER_PIN    = 15;
  static constexpr int BUTTON_PIN    = 13;

  // Debounce
  bool readButtonPressed();

  // LEDs & buzzer helpers
  void setGreen(bool on);
  void setRed(bool on);
  void setBuzzer(bool on);

  // Scheduling
  void computeTodaySchedule();           // needs local time + sunset
  void ensureTimeAndSunsetIfNeeded();    // WiFi + NTP + Sunset

  // Time helpers
  static time_t makeTodayAt(int hh, int mm, int ss = 0);
  static time_t clampBefore(time_t a, time_t b) { return (a < b) ? a : b; }
  static time_t clampAfter(time_t a, time_t b)  { return (a > b) ? a : b; }

  // Daily targets (all are LOCAL time epoch seconds)
  time_t tMorningRedOn      = 0;   // 08:45
  time_t tMorningAlarm      = 0;   // 09:30
  time_t tNapStart          = 0;   // 11:00
  time_t tNapEnd            = 0;   // 15:00
  time_t tAfternoonGreen    = 0;   // 15:00
  time_t tEveningRedOn      = 0;   // min(sunset-1h, 22:00)
  time_t tEveningAlarm      = 0;   // min(sunset+30min, 22:30)
  time_t tMidnightSleep     = 0;   // today at 00:00 (for reference)
  time_t tNextMorningWake   = 0;   // next day 08:00 (for reference)

  // Flow flags
  bool scheduleReady        = false;
  bool morningAcknowledged  = false;
  bool eveningAcknowledged  = false;

  // Button
  uint32_t lastBtnChangeMs  = 0;
  bool lastBtnStable        = true;

  // State
  State state               = SLEEP;

  // One-time daily reset
  int lastScheduleYMD       = -1;   // YYYYMMDD stored to detect a new day
};
