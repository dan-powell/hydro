/*********
  Basic Hydroponics Pump Timer
  Uses ESP32 time library to manage system clock: https://github.com/fbiego/ESP32Time

  TODO

  // Pump Timer
  // Define an array of times (hours?) for the pump to run




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

ESP32Time rtc;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

#define RELAY_PIN_1 17 // Pin for relay 1

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define LOGO_HEIGHT   64
#define LOGO_WIDTH    64

#include <bitmaps.h>

int pump_times[] = {10, 11, 14, 16, 17, 18};
int pump_times_length = 6;
bool pump_times_state[] = {true, true, true, true, true, true};
int pump_seconds = 10;

bool pump_times_state_temp[6];

void debug(String line) {
  // Serial output
  Serial.println(line);
  // Display
  display.clearDisplay();
  display.setCursor(0, 10);
  display.setTextColor(WHITE);
  display.println(line);
  display.display();
  delay(500);
}

void timer_setup(void) {
  for (int i = 0; i < pump_times_length; i++) {     
    pump_times_state_temp[i] = pump_times_state[i];     
  }
}

bool timer_getstate(int hour) {
  int wantedpos;
  for (int i=0; i<pump_times_length; i++) {
    if (hour == pump_times[i]) {
      wantedpos = i;
      break;
    }
  }
  return pump_times_state_temp[wantedpos];
}

bool timer_setstate(int hour, bool value) {
  int wantedpos;
  for (int i=0; i<pump_times_length; i++) {
    if (hour == pump_times[i]) {
      wantedpos = i;
      break;
    }
  }
  pump_times_state_temp[wantedpos] = value;
}

int timer_next(void) {
  int hour = rtc.getHour(true);
  for (int i=0; i<pump_times_length; i++) {
    if (pump_times[i] >= hour) {
      bool state = timer_getstate(pump_times[i]);
      if(state == true) {
        return pump_times[i];
        break;
      }
    }
  }
  return 0;
}

void pump_run(void) {
  // Turn relay on
  debug("Running Pump");
  digitalWrite(RELAY_PIN_1, HIGH);
  delay(pump_seconds * 1000);
  // Turn relay off
  digitalWrite(RELAY_PIN_1, LOW);
}

void timer_check(void) {
  int hour = rtc.getHour(true);
  int next_hour = timer_next();
  // Check if we have reached next time
  if (hour == next_hour) {
    // Check if time has run or not
    bool state = timer_getstate(next_hour);
    if (state == true) {
      pump_run();
      timer_setstate(next_hour, false);
    }
  }
}

void updateTimeNtp(void) {
  
  display.clearDisplay();

  // Connect to Wi-Fi
  debug("Connecting to Wifi: " + String(wifi_ssid));
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  debug("Wifi Connected");
  
  // Init and get the time
  debug("Updating Time");
  struct tm timeinfo;
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  getLocalTime(&timeinfo);
  rtc.setTimeStruct(timeinfo);
  debug("Time Updated");
  display.println(&timeinfo, "%d %Y %H:%M:%S");

  // Disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  debug("Wifi Disconnected");

  display.clearDisplay();

}

void draw_time(void) {
  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  struct tm timeinfo = rtc.getTimeStruct();
  display.println(&timeinfo, "%H:%M:%S");
}

void draw_pump(void) {
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 20);
  int hour = timer_next();
  display.println(hour);
}


void setup() {
  Serial.begin(115200);

  // Set default pin status
  pinMode(RELAY_PIN_1, OUTPUT);
  digitalWrite(RELAY_PIN_1, LOW);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Display boot logo
  display.clearDisplay();
  display.drawBitmap(32, 0, bmp_logo, 64, 64, 1);
  display.display();
  delay(4000);

  // Update system clock from NTP
  updateTimeNtp();
  timer_setup();
}

void loop() {
  display.clearDisplay();
  timer_check();
  draw_time();
  draw_pump();
  display.display();
  delay(500);
}
