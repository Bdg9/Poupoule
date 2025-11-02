#pragma once
#include <time.h>

inline time_t makeTodayAt(int hour, int minute, int second = 0) {
  time_t now = time(nullptr);     // current time (epoch)
  struct tm lt;
  localtime_r(&now, &lt);           // convert to local time components

  lt.tm_hour = hour;
  lt.tm_min  = minute;
  lt.tm_sec  = second;

  // mktime() converts back to epoch seconds (local time)
  return mktime(&lt);
}