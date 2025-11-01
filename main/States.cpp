#include "States.h"
#include <WiFi.h>
#include <HTTPClient.h>

// ---- Private helpers ----
static int ymdFromLocal(time_t t) {
  struct tm lt;
  localtime_r(&t, &lt);
  return (lt.tm_year + 1900) * 10000 + (lt.tm_mon + 1) * 100 + lt.tm_mday;
}

void StateMachine::begin() {
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  setGreen(false);
  setRed(false);
  setBuzzer(false);

  if (!hasWifiCredentials()) {
    Serial.println(F("[WARN] No WiFi credentials set in Config.cpp"));
  }

  // Always try to sync on boot (we wake at ~08:00 after night's sleep in logic)
  ensureTimeAndSunsetIfNeeded();
  computeTodaySchedule();

  // Start the day depending on current clock
  state = SLEEP; // default; update() will step into the right one
}

bool StateMachine::readButtonPressed() {
  // Active LOW button with debounce (~30 ms)
  bool raw = (digitalRead(BUTTON_PIN) == LOW);
  static bool stable = false;

  uint32_t now = millis();
  if (raw != lastBtnStable) {
    lastBtnChangeMs = now;
    lastBtnStable = raw;
  }
  if (now - lastBtnChangeMs > 30) {
    stable = raw;
  }
  return stable;
}

void StateMachine::setGreen(bool on)  { digitalWrite(GREEN_LED_PIN, on ? HIGH : LOW); }
void StateMachine::setRed(bool on)    { digitalWrite(RED_LED_PIN,   on ? HIGH : LOW); }
void StateMachine::setBuzzer(bool on) { digitalWrite(BUZZER_PIN,    on ? HIGH : LOW); }

time_t StateMachine::makeTodayAt(int hh, int mm, int ss) {
  time_t now = time(nullptr);
  struct tm lt;
  localtime_r(&now, &lt);
  lt.tm_hour = hh; lt.tm_min = mm; lt.tm_sec = ss;
  return mktime(&lt); // returns local epoch seconds
}

void StateMachine::ensureTimeAndSunsetIfNeeded() {
  if (scheduleReady) return;

  if (WiFi.status() != WL_CONNECTED) {
    connectWifi();
  }

  if (!syncTimeViaNTP(getTZString())) {
    Serial.println(F("[ERROR] NTP sync failed."));
    state = ERRROR;
    return;
  }

  // Sunset: fetch today in UTC then convert to local
  time_t sunsetUtc;
  if (!fetchTodaySunsetUTC(getLatitude(), getLongitude(), &sunsetUtc)) {
    Serial.println(F("[ERROR] Sunset fetch failed."));
    state = ERRROR;
    return;
  }

  // Store as local epoch seconds for convenience
  // Convert UTC->local by applying localtime offset via tm/mktime trick:
  struct tm lt;
  localtime_r(&sunsetUtc, &lt);
  time_t sunsetLocal = mktime(&lt);

  // Cache in globals via the schedule routine (we don't store the raw sunset here)
  (void)sunsetLocal;

  scheduleReady = true;
}

void StateMachine::computeTodaySchedule() {
  // Needs valid local time and working TZ
  time_t now = time(nullptr);
  int today = ymdFromLocal(now);
  if (today == lastScheduleYMD && tMorningRedOn != 0) return;

  // We will recompute sunset now (ensureTimeAndSunsetIfNeeded fetched it already)
  // Fetch again to get the actual sunsetLocal:
  time_t sunsetUtc;
  if (!fetchTodaySunsetUTC(getLatitude(), getLongitude(), &sunsetUtc)) {
    state = ERRROR;
    return;
  }
  struct tm tmp;
  localtime_r(&sunsetUtc, &tmp);
  time_t sunsetLocal = mktime(&tmp);

  // Fixed anchors
  tMorningRedOn   = makeTodayAt(8, 45, 0);
  tMorningAlarm   = makeTodayAt(9, 30, 0);
  tNapStart       = makeTodayAt(11, 0, 0);
  tNapEnd         = makeTodayAt(15, 0, 0);
  tAfternoonGreen = tNapEnd;
  time_t t22_00   = makeTodayAt(22, 0, 0);
  time_t t22_30   = makeTodayAt(22, 30, 0);

  // Evening windows relative to sunset
  time_t eveningRedBySun   = sunsetLocal - 3600;      // sunset - 1h
  time_t eveningAlarmBySun = sunsetLocal + (30 * 60); // sunset + 30 min

  // Apply "latest/earliest" rules
  tEveningRedOn = (eveningRedBySun < t22_00) ? eveningRedBySun : t22_00;
  tEveningAlarm = (eveningAlarmBySun < t22_30) ? eveningAlarmBySun : t22_30;

  // Midnight reference and next morning 08:00
  tMidnightSleep   = makeTodayAt(0, 0, 0);
  // next day 08:00:
  struct tm tmNext;
  localtime_r(&now, &tmNext);
  tmNext.tm_mday += 1; tmNext.tm_hour = 8; tmNext.tm_min = 0; tmNext.tm_sec = 0;
  tNextMorningWake = mktime(&tmNext);

  morningAcknowledged = false;
  eveningAcknowledged = false;

  lastScheduleYMD = today;

  Serial.println(F("[INFO] Schedule computed:"));
  Serial.printf("  Morning red on:   %s", ctime(&tMorningRedOn));
  Serial.printf("  Morning alarm:    %s", ctime(&tMorningAlarm));
  Serial.printf("  Nap start:        %s", ctime(&tNapStart));
  Serial.printf("  Nap end:          %s", ctime(&tNapEnd));
  Serial.printf("  Afternoon green:  %s", ctime(&tAfternoonGreen));
  Serial.printf("  Evening red on:   %s", ctime(&tEveningRedOn));
  Serial.printf("  Evening alarm:    %s", ctime(&tEveningAlarm));
  Serial.printf("  Night sleep ->8h: %s", ctime(&tNextMorningWake));
}

void StateMachine::update() {
  // Recompute schedule at midnight boundary or if not ready
  time_t now = time(nullptr);
  if (!scheduleReady) {
    ensureTimeAndSunsetIfNeeded();
    if (!scheduleReady) return;
    computeTodaySchedule();
  }
  int today = ymdFromLocal(now);
  if (today != lastScheduleYMD) {
    // New day rollover
    scheduleReady = false;
    ensureTimeAndSunsetIfNeeded();
    computeTodaySchedule();
  }

  // Global time-driven state gating (simple scheduler)
  // 00:00 -> 08:00 : "night sleep"
  // 08:45 -> 11:00 : morning open task
  // 11:00 -> 15:00 : nap (sleep window)
  // 15:00 -> eveningRedOn : ALLGOOD (green)
  // eveningRedOn -> midnight : close task
  struct tm lt;
  localtime_r(&now, &lt);
  int hour = lt.tm_hour;

  // Night sleep window (00:00 .. 08:00)
  if (now >= tMidnightSleep && now < makeTodayAt(8, 0, 0)) {
    // Sleep: LEDs and buzzer OFF
    if (state != SLEEP) {
      setGreen(false); setRed(false); setBuzzer(false);
      state = SLEEP;
      Serial.println(F("[STATE] SLEEP (night)"));
    }
    delay(50);
    return;
  }

  // 08:00: (re)sync WiFi/time/sunset if needed (once in the morning)
  if (hour == 8 && !scheduleReady) {
    ensureTimeAndSunsetIfNeeded();
    computeTodaySchedule();
  }

  bool button = readButtonPressed();

  // MORNING OPEN: 08:45 -> 11:00
  if (now >= tMorningRedOn && now < tNapStart) {
    if (!morningAcknowledged) {
      // We are waiting for the "open the chickens" action.
      if (now < tMorningAlarm) {
        // Early waiting phase (no buzzer)
        if (state != CLOSEDOOR) {
          setRed(true); setGreen(false); setBuzzer(false);
          state = CLOSEDOOR;
          Serial.println(F("[STATE] CLOSEDOOR (morning waiting)"));
        }
      } else {
        // Alarm window
        if (state != NOBODYCLOSEDIT) {
          setRed(true); setGreen(false); 
          state = NOBODYCLOSEDIT;
          Serial.println(F("[STATE] NOBODYCLOSEDIT (morning alarm)"));
        }
        // Simple continuous alarm; (could be pulsed if desired)
        setBuzzer(true);
      }

      if (button) {
        morningAcknowledged = true;
        setBuzzer(false);
        setRed(false); setGreen(true);
        state = ALLGOOD;
        Serial.println(F("[EVENT] Morning acknowledged -> ALLGOOD"));
      }
      return;
    }
  }

  // 11:00 -> 15:00 : Nap window
  if (now >= tNapStart && now < tNapEnd) {
    if (state != SLEEP) {
      setGreen(false); setRed(false); setBuzzer(false);
      state = SLEEP;
      Serial.println(F("[STATE] SLEEP (midday nap)"));
    }
    delay(50);
    return;
  }

  // 15:00 -> eveningRedOn : All good (green)
  if (now >= tAfternoonGreen && now < tEveningRedOn) {
    if (state != ALLGOOD) {
      setBuzzer(false);
      setRed(false); setGreen(true);
      state = ALLGOOD;
      Serial.println(F("[STATE] ALLGOOD (afternoon)"));
    }
    return;
  }

  // EVENING CLOSE: eveningRedOn -> midnight
  if (now >= tEveningRedOn && now < makeTodayAt(23,59,59)) {
    if (!eveningAcknowledged) {
      if (now < tEveningAlarm) {
        if (state != CLOSEDOOR) {
          setRed(true); setGreen(false); setBuzzer(false);
          state = CLOSEDOOR;
          Serial.println(F("[STATE] CLOSEDOOR (evening waiting)"));
        }
      } else {
        if (state != NOBODYCLOSEDIT) {
          setRed(true); setGreen(false);
          state = NOBODYCLOSEDIT;
          Serial.println(F("[STATE] NOBODYCLOSEDIT (evening alarm)"));
        }
        setBuzzer(true);
      }

      if (button) {
        eveningAcknowledged = true;
        setBuzzer(false);
        setRed(false); setGreen(true);
        state = ALLGOOD;
        Serial.println(F("[EVENT] Evening acknowledged -> ALLGOOD"));
      }
      return;
    } else {
      // Already acknowledged -> keep green on
      if (state != ALLGOOD) {
        setBuzzer(false);
        setRed(false); setGreen(true);
        state = ALLGOOD;
        Serial.println(F("[STATE] ALLGOOD (evening acknowledged)"));
      }
      return;
    }
  }

  // Fallback: keep green as default during the day outside windows
  if (state != ALLGOOD) {
    setBuzzer(false);
    setRed(false); setGreen(true);
    state = ALLGOOD;
  }
}
