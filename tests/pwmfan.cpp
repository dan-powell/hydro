/*********
  Basic Hydroponics Pump Timer
  Uses ESP32 time library to manage system clock: https://github.com/fbiego/ESP32Time

  TODO

* Fix issue with hour == 0 & reset

*********/

#include <config.h>
#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <ESP32Time.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

int pinFan1Rpm = 17; // TX2
int freq = 25000;
int resolution = 8;

void setup() {
  ledcAttach(pinFan1Rpm, freq, resolution);
  ledcWrite(pinFan1Rpm, 255);
}

void loop(){
  // increase the LED brightness
  for(int dutyCycle = 0; dutyCycle <= 255; dutyCycle++){   
    // changing the LED brightness with PWM
    ledcWrite(pinFan1Rpm, dutyCycle);
    delay(15);
  }

  // decrease the LED brightness
  for(int dutyCycle = 255; dutyCycle >= 0; dutyCycle--){
    // changing the LED brightness with PWM
    ledcWrite(pinFan1Rpm, dutyCycle);   
    delay(15);
  }
}