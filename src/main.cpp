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

const int pwm_channel_fan = 1;
const int pwm_channel_light1 = 2;
const int pwm_channel_light2 = 3;

int pwm_channel_fan_status = 0;
int pwm_channel_light1_status = 0;
int pwm_channel_light2_status = 0;

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
  ledcWrite(pwm_channel_fan, duty);  
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
        ledcWrite(pwm_channel_fan, fan_times[i][1]);
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
        if (light_times[i][1] == 0) {
          ledcWrite(pwm_channel_light1, light_times[i][2]);
          ledcWrite(pwm_channel_light2, light_times[i][2]);
          pwm_channel_light1_status = light_times[i][2];
          pwm_channel_light2_status = light_times[i][2];
        }
        if (light_times[i][1] == 1) {
          ledcWrite(pwm_channel_light1, light_times[i][2]);
          pwm_channel_light1_status = light_times[i][2];
        }
        if (light_times[i][1] == 2) {
          ledcWrite(pwm_channel_light2, light_times[i][2]);
          pwm_channel_light2_status = light_times[i][2];
        }
        light_times_status[i] = false;
      }
    }
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
  ledcSetup(pwm_channel_fan, pwm_frequency, pwm_resolution);
  ledcAttachPin(PIN_FAN, pwm_channel_fan);
  ledcWrite(pwm_channel_fan, 0);
  // Lights 1 & 2
  ledcSetup(pwm_channel_light1, pwm_frequency, pwm_resolution);
  ledcAttachPin(PIN_LIGHT1, pwm_channel_light1);
  ledcWrite(pwm_channel_light1, 0);
  ledcSetup(pwm_channel_light2, pwm_frequency, pwm_resolution);
  ledcAttachPin(PIN_LIGHT2, pwm_channel_light2);
  ledcWrite(pwm_channel_light2, 0);

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
}

void loop() {
  display.clearDisplay();
  timer_pump_check();
  timer_fan_check();
  timer_light_check();
  draw_time();
  draw_pump();
  display.display();
  reset_check();
  delay(100);
}
