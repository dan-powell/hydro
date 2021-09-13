/*********
  Basic Hydroponics Pump Timer
  Uses ESP32 time library to manage system clock: https://github.com/fbiego/ESP32Time

  TODO

* Fix issue with hour == 0 & reset
* Handle incorrect time or not being able to update from wifi


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

#define RELAY_PIN_1 17 // Pin for relay 1

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define LOGO_HEIGHT   64
#define LOGO_WIDTH    64

#include <bitmaps.h>

int pump_times_length = 0;
bool pump_times_status[24];

void debug(String line, String line2 = "") {
  // Serial output
  Serial.println(line);
  // Display
  display.clearDisplay();
  display.setCursor(0, 10);
  display.setTextColor(WHITE);
  display.println(line);
  display.println(line2);
  display.display();
  delay(500);
}

// Setup/Reset the timers for the day
void timer_setup(void) {
  // Get the length of pump_times array for looping later
  pump_times_length = sizeof(pump_times)/sizeof(pump_times[0]);
  // Reset the status of all pumping times (true = ready)
  for (int i = 0; i < pump_times_length; i++) {     
    pump_times_status[i] = true;     
  }
}

// Get the state of a particular time
bool timer_getstate(int time) {
  // Find the index that matches hour
  int index = -1;
  for (int i=0; i<pump_times_length; i++) {
    if (time == pump_times[i]) {
      index = i;
      break;
    }
  }
  if (index == -1) {
    return true;
  } else {
    // Return current status
    return pump_times_status[index];
  }
}

// Set the state of a time
void timer_setstate(int time, bool value) {
  int index = -1;
  // Find the index that matches hour
  for (int i=0; i<pump_times_length; i++) {
    if (time == pump_times[i]) {
      index = i;
      break;
    }
  }
  if (index != -1) {
    // Update status as per value
    pump_times_status[index] = value;
  }
}

// Get the next timer that is ready
int timer_next(void) {
  int hour = rtc.getHour(true);
  int minute = rtc.getMinute();
  int time = (hour * 60) + minute;
  for (int i=0; i<pump_times_length; i++) {
    if (pump_times[i] >= time) {
      bool state = timer_getstate(pump_times[i]);
      if(state == true) {
        return pump_times[i];
        break;
      }
    }
  }
  // No time was found, so check we are at end of day. 
  if(time == 0) {
    // Reset Timers
    timer_setup();
  }
  return 0;
}

// Run the pump
void pump_run(void) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Watering");
  display.display();

  // Turn relay on
  digitalWrite(RELAY_PIN_1, HIGH);
  delay(pump_duration * 1000);
  // Turn relay off
  digitalWrite(RELAY_PIN_1, LOW);
}

// Check if we need to run the next time
void timer_check(void) {
  int hour = rtc.getHour(true);
  int minute = rtc.getMinute();
  int time = (hour * 60) + minute;
  int next_time = timer_next();
  // Check if we have reached next time
  if (time == next_time) {
    // Check if time has run or not
    bool state = timer_getstate(next_time);
    if (state == true) {
      pump_run();
      timer_setstate(next_time, false);
    }
  }
}

void updateTimeNtp(void) {
  
  display.clearDisplay();

  // Connect to Wi-Fi
  debug("Connecting to Wifi: ", String(wifi_ssid));
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  debug("Wifi connected");
  
  // Init and get the time
  debug("Updating time");
  struct tm timeinfo;
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  getLocalTime(&timeinfo);
  rtc.setTimeStruct(timeinfo);

  if (rtc.getHour() == 0 && rtc.getMinute() == 0) {
    debug("Time invalid - Try again");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    getLocalTime(&timeinfo);
    rtc.setTimeStruct(timeinfo);
  }

  debug("Time updated");
  display.println(&timeinfo, "%d %Y %H:%M:%S");

  // Disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  debug("Wifi disconnected");

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
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 30);
  display.println("Next watering:");
  display.setCursor(0, 50);
  display.setTextSize(2);
  int time = timer_next();
  if(time == 0) {
    display.println("Tomorrow");
  } else {
    int hour = floor(time/60);
    int minute = round((((float)time/60) - (float)hour) * 60);
    if(minute == 0) {
      display.println(String(hour) + ":00");
    } else if(minute < 10) {
      display.println(String(hour) + ":0" + String(minute));
    } else {
      display.println(String(hour) + ":" + String(minute));
    }
  }
  
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
  delay(100);
}
