#include "States.h"

StateMachine sm;

void setup() {
  Serial.begin(115200);
  sm.begin();
}

void loop() {
  sm.update();
}
