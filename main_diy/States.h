#pragma once
#include <Arduino.h>
#include "Config.h"

enum class States {
    IDLE,
    ALLGOOD,
    CLOSEDOOR,
    NOBODYCLOSEDIT,
    ERROR
};

class StateMachine {
public:
    void begin();
    void update();
private:
    States currentState = States::IDLE;

    // Debounce
    bool readButtonPressed();

    // LEDs & buzzer helpers
    void setGreen(bool on);
    void setRed(bool on);
    void setBuzzer(bool on);

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

};
