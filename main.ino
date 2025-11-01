#include <Arduino.h>
#include "States.h"
#include "Config.h"

void setup() {
  // Initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_BUTTON, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
}

void loop() {
  // Print "Hello, World!" to the Serial Monitor
  Serial.println("Hello, World!");
  // Wait for a second
  delay(1000);
}