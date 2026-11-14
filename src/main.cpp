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

ESP32Time rtc;

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define LOGO_HEIGHT   64
#define LOGO_WIDTH    64

#include <bitmaps.h>

// Setup some variables to hold status
int pump_times_length = 0;
bool pump_times_status[100];

int light_times_length = 0;
bool light_times_status[100];

int fan_times_length = 0;
bool fan_times_status[1440];

int pwm_channel_fan_status = 0;
int pwm_channel_light1_status = 0;
int pwm_channel_light2_status = 0;

// Levels the lights should fade to (see lights_update)
int light1_target = 0;
int light2_target = 0;

bool reset = false;

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
void timers_setup(void) {
  // Get the length of pump_times array for looping later
  pump_times_length = sizeof(pump_times)/sizeof(pump_times[0]);
  light_times_length = sizeof(light_times)/sizeof(light_times[0]);
  fan_times_length = sizeof(fan_times)/sizeof(fan_times[0]);

  // Reset the status of all timers (true = ready)
  for (int i = 0; i < pump_times_length; i++) {     
    pump_times_status[i] = true;     
  }
  for (int i = 0; i < light_times_length; i++) {     
    light_times_status[i] = true;     
  }
  for (int i = 0; i < fan_times_length; i++) {     
    fan_times_status[i] = true;     
  }
}

// Get the state of a particular time
bool timer_pump_getstate(int time) {
  // Find the index that matches hour
  int index = -1;
  for (int i=0; i<pump_times_length; i++) {
    if (time == pump_times[i]) {
      index = i;
      break;
    }
  }
  if (index == -1) {
    return false;
  } else {
    // Return current status
    return pump_times_status[index];
  }
}

// Set the state of a time
void timer_pump_setstate(int time, bool value) {
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
int timer_pump_next(void) {
  int hour = rtc.getHour(true);
  int minute = rtc.getMinute();
  int time = (hour * 60) + minute;
  for (int i=0; i<pump_times_length; i++) {
    if (pump_times[i] >= time) {
      bool state = timer_pump_getstate(pump_times[i]);
      if(state == true) {
        return pump_times[i];
        break;
      }
    }
  }
  return -1;
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
  digitalWrite(PIN_PUMP, HIGH);
  delay(pump_duration * 1000);
  // Turn relay off
  digitalWrite(PIN_PUMP, LOW);
}

void fan_update(int duty) {
  ledcWrite(PIN_FAN, duty);  
}

// Check if we need to run the next time
void timer_pump_check(void) {
  int hour = rtc.getHour(true);
  int minute = rtc.getMinute();
  int time = (hour * 60) + minute;
  int next_time = timer_pump_next();
  // Check if we have reached next time
  if (time == next_time) {
    // Check if time has run or not
    bool state = timer_pump_getstate(next_time);
    if (state == true) {
      pump_run();
      timer_pump_setstate(next_time, false);
    }
  }
}

int get_time_minutes() {
  int hour = rtc.getHour(true);
  int minute = rtc.getMinute();
  return (hour * 60) + minute;
}


void timer_fan_check(void) {
  int time = get_time_minutes();

  // Loop over times
  for (int i=0; i<fan_times_length; i++) {
    if (fan_times[i][0] == time) {
      if (fan_times_status[i] == true) {
        ledcWrite(PIN_FAN, fan_times[i][1]);
        pwm_channel_fan_status = fan_times[i][1];
        fan_times_status[i] = false;
      }
    }
  }
}


void timer_light_check(void) {
  int time = get_time_minutes();
  for (int i=0; i<light_times_length; i++) {
    if (light_times[i][0] == time) {
      if (light_times_status[i] == true) {
        // Set the targets only - lights_update does the fading
        if (light_times[i][1] == 0 || light_times[i][1] == 1) {
          light1_target = light_times[i][2];
        }
        if (light_times[i][1] == 0 || light_times[i][1] == 2) {
          light2_target = light_times[i][2];
        }
        light_times_status[i] = false;
      }
    }
  }
}


// Start fading a light towards its target, if it isn't already there.
// The original ESP32 can't interrupt a running hardware fade, so if one is
// still going ledcFade fails and we try again on the next loop.
void light_fade_to(int pin, int &status, int target) {
  if (status == target) return;
  if (light_fade_ms <= 0) {
    ledcWrite(pin, target);
    status = target;
  } else if (ledcFade(pin, status, target, light_fade_ms)) {
    status = target;
  }
}

void lights_update(void) {
  light_fade_to(PIN_LIGHT1, pwm_channel_light1_status, light1_target);
  light_fade_to(PIN_LIGHT2, pwm_channel_light2_status, light2_target);
}


// Work out the duty the fan should be at for a given time, from the most
// recent schedule entry. If nothing has happened yet today, carry over the
// last entry of the day (i.e. what was set yesterday evening).
// Returns 0 if there are no entries.
int fan_duty_at(int time) {
  int best_today = -1, best_today_time = -1;
  int best_any = -1, best_any_time = -1;
  for (int i=0; i<fan_times_length; i++) {
    int t = fan_times[i][0];
    // >= so that later entries win ties, matching timer_fan_check
    if (t >= best_any_time) { best_any_time = t; best_any = i; }
    if (t <= time && t >= best_today_time) { best_today_time = t; best_today = i; }
  }
  int index = (best_today != -1) ? best_today : best_any;
  return (index == -1) ? 0 : fan_times[index][1];
}

// As fan_duty_at, for light 1 or 2. Entries for "both" (0) count for either.
int light_duty_at(int light, int time) {
  int best_today = -1, best_today_time = -1;
  int best_any = -1, best_any_time = -1;
  for (int i=0; i<light_times_length; i++) {
    if (light_times[i][1] != 0 && light_times[i][1] != light) continue;
    int t = light_times[i][0];
    if (t >= best_any_time) { best_any_time = t; best_any = i; }
    if (t <= time && t >= best_today_time) { best_today_time = t; best_today = i; }
  }
  int index = (best_today != -1) ? best_today : best_any;
  return (index == -1) ? 0 : light_times[index][2];
}

// On boot, put the fan and lights into the state the schedule says they
// should be in now, and mark today's earlier entries as done.
// The pump is deliberately not caught up - missed waterings are skipped.
void restore_state(void) {
  int time = get_time_minutes();

  pwm_channel_fan_status = fan_duty_at(time);
  ledcWrite(PIN_FAN, pwm_channel_fan_status);
  // Lights fade up from off to their current level
  light1_target = light_duty_at(1, time);
  light2_target = light_duty_at(2, time);
  lights_update();

  for (int i=0; i<fan_times_length; i++) {
    if (fan_times[i][0] <= time) fan_times_status[i] = false;
  }
  for (int i=0; i<light_times_length; i++) {
    if (light_times[i][0] <= time) light_times_status[i] = false;
  }
}


// Check if we need to reset the timers
void reset_check(void) {
  // get the time elapsed in minutes
  int hour = rtc.getHour(true);
  int minute = rtc.getMinute();
  int time = (hour * 60) + minute;
  // just after midnight
  if(time == 1 && reset == false) {
    // Reset Timers
    debug("Resetting timers");
    timers_setup();
    reset = true;
  }
  // reset the reset
  if(time == 2) {
    reset = false;
  }
}

void getTimeNTP(void) {
  
  debug("Updating time");
  struct tm timeinfo;
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  getLocalTime(&timeinfo);
  rtc.setTimeStruct(timeinfo);

  // Check if the time is valid
  if (rtc.getHour() == 0 && rtc.getMinute() == 0) {
    debug("Invalid time - retry");
    delay(5000);
    getTimeNTP();
  }

  debug("Time updated");

}

// Init and get the time
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

  // Retrieve time from NTP server
  getTimeNTP();
  
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
  bool water = digitalRead(PIN_WATER);
  if(water) {
    display.println("~ OK");
  } else {
    display.println(" ~ LOW");
  }

  display.setTextSize(1);
  display.setCursor(0, 20);
  display.println("Light 1 " + String((int)floor((float)pwm_channel_light1_status/255*100)) + "%");
  display.print("Light 2 " + String((int)floor((float)pwm_channel_light2_status/255*100)) + "%");
  display.setCursor(80, 20);
  display.println("Fan " + String((int)floor((float)pwm_channel_fan_status/255*100)) + "%");
}

void draw_pump(void) {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 39);
  struct tm timeinfo = rtc.getTimeStruct();
  display.print("Time: ");
  display.print(&timeinfo, "%H:%M:%S");
  display.setCursor(0, 48);
  display.println("Next");
  display.println("Water:");
  display.setCursor(40, 50);
  display.setTextSize(2);
  int time = timer_pump_next();
  if(time == -1) {
    display.print("Tomorrow");
  } else {
    int hour = floor(time/60);
    int minute = round((((float)time/60) - (float)hour) * 60);
    if(minute == 0) {
      display.print(String(hour) + ":00");
    } else if(minute < 10) {
      display.print(String(hour) + ":0" + String(minute));
    } else {
      display.print(String(hour) + ":" + String(minute));
    }
  }
  
}


void setup() {
  Serial.begin(115200);

  // Set default pin status
  pinMode(PIN_PUMP, OUTPUT);
  digitalWrite(PIN_PUMP, LOW);
  pinMode(PIN_WATER, INPUT);

  // Set PWM pins
  // Fan
  ledcAttach(PIN_FAN, pwm_frequency, pwm_resolution);
  ledcWrite(PIN_FAN, 0);
  // Lights 1 & 2
  ledcAttach(PIN_LIGHT1, pwm_frequency, pwm_resolution);
  ledcWrite(PIN_LIGHT1, 0);
  ledcAttach(PIN_LIGHT2, pwm_frequency, pwm_resolution);
  ledcWrite(PIN_LIGHT2, 0);

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
  timers_setup();
  // Catch up fan and lights in case we rebooted part way through the day
  restore_state();
}

void loop() {
  display.clearDisplay();
  timer_pump_check();
  timer_fan_check();
  timer_light_check();
  lights_update();
  draw_time();
  draw_pump();
  display.display();
  reset_check();
  delay(100);
}
