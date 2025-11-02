#include "States.h"
#include "Config.h"
#include "Helpers.h"

void StateMachine::begin() {
    //set pinmodes
    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    //default schedule
    tMorningRedOn  = makeTodayAt(MORNINGREDON_H, MORNINGREDON_M, 0);
    tMorningAlarm  = makeTodayAt(MORNINGALARM_H, MORNINGALARM_M, 0);
    tNapStart      = makeTodayAt(NAPSTART_H, NAPSTART_M, 0);
    tNapEnd        = makeTodayAt(NAPEND_H, NAPEND_M, 0);

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

void StateMachine::handleIdleState(time_t now) {
    // Handle IDLE state
    if (now >= tMorningRedOn && now < tMorningAlarm) {
        setRed(true); setGreen(false); setBuzzer(false);
        currentState = States::CLOSEDOOR;
    } else if (now >= tEveningRedOn && now < tEveningAlarm) {
        setRed(true); setGreen(false); setBuzzer(false);
        currentState = States::CLOSEDOOR;
    } else {
        setGreen(false); setRed(false); setBuzzer(false);
    }
}

void StateMachine::handleAllGoodState(time_t now) {
    // Handle ALLGOOD state

}

void StateMachine::handleCloseDoorState(time_t now) {
    // Handle CLOSEDOOR state
    if (readButtonPressed()) {
        currentState = States::ALLGOOD;
        setRed(false); setGreen(true); setBuzzer(false);
    } else if (now >= tMorningAlarm && now < tNapStart) {
        currentState = States::NOBODYCLOSEDIT;
        setBuzzer(true);
    } else if (now >= tEveningAlarm && now < makeTodayAt(23,59,59)) {
        currentState = States::NOBODYCLOSEDIT;
        setBuzzer(true);
    }
}

void StateMachine::handleNobodyClosedItState(time_t now) {
    // Handle NOBODYCLOSEDIT state
    if (readButtonPressed()) {
        currentState = States::ALLGOOD;
        setRed(false); setGreen(true); setBuzzer(false);
    }
}

void StateMachine::handleErrorState(time_t now) {
    // Handle ERROR state
}

void StateMachine::update() {
    time_t now = time(nullptr); // TODO: get current time
    switch (currentState) {
        case States::IDLE:
            handleIdleState(now);
            break;
        case States::ALLGOOD:
            handleAllGoodState(now);
            break;
        case States::CLOSEDOOR:
            handleCloseDoorState(now);
            break;
        case States::NOBODYCLOSEDIT:
            handleNobodyClosedItState(now);
            break;
        case States::ERROR:
            handleErrorState(now);
            break;
    }
}
