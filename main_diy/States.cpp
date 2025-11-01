#include "States.h"

void StateMachine::begin() {
    //set pinmodes
    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    currentState = States::IDLE;
}

bool StateMachine::readButtonPressed() {
    // Active LOW button with debounce (~30 ms)
    bool raw = (digitalRead(BUTTON_PIN) == LOW);
    static bool stable = false;

    static uint32_t lastBtnChangeMs = 0;
    static bool lastBtnStable = raw;

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

void StateMachine::update() {
    switch (currentState) {
        case States::IDLE:
            // Handle IDLE state
            break;
        case States::ALLGOOD:
            // Handle ALLGOOD state
            break;
        case States::CLOSEDOOR:
            // Handle CLOSEDOOR state
            break;
        case States::NOBODYCLOSEDIT:
            // Handle NOBODYCLOSEDIT state
            break;
        case States::ERROR:
            // Handle ERROR state
            break;
    }
}
