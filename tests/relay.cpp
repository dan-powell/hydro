#include <Arduino.h>

#define RELAY_PIN_1 17 // Pin for relay 1

void setup() {
    Serial.begin(115200);
    pinMode(RELAY_PIN_1, OUTPUT);
}

void loop() {
    // Alternate between on and off state
    delay(1000);
    digitalWrite(RELAY_PIN_1, LOW);
    delay(1000);
    digitalWrite(RELAY_PIN_1, HIGH);
}
