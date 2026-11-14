// Copy this file to src/config.h and fill in your own values.
// config.h is ignored by git so your WiFi credentials stay private.

// All schedule times below are minutes since 0:00
// e.g 11:00 -> 11 * 60 = 660
// e.g 14:30 -> 14 * 60 + 30 = 870
// FIX - Don't use 0 (midnight) as a time; it will cause problems with the daily reset

// Wifi Settings
const char* wifi_ssid       = "";
const char* wifi_password   = "";

// Time settings
const char* ntpServer = "pool.ntp.org"; // NTP server to use
const long  gmtOffset_sec = 0; // Timezone offset
const int   daylightOffset_sec = 3600; // Daylight Savings Offset

// Pin assignments (GPIO numbers) - check these match your wiring
// Avoid GPIO 21/22 (I2C display) and strapping pins 0, 2, 5, 12, 15
#define PIN_PUMP    16 // Relay for the water pump (RX2)
#define PIN_WATER   27 // Water level sensor (digital, HIGH = OK). Needs an external pull-up/down
#define PIN_FAN     17 // Fan PWM (TX2)
#define PIN_LIGHT1  18 // Light 1 PWM (D18)
#define PIN_LIGHT2  19 // Light 2 PWM (D19)

// PWM settings (shared by fan and lights)
// 25kHz is the standard for 4-pin PC fans and is above audible range
const int pwm_frequency  = 25000; // Hz
const int pwm_resolution = 8;     // bits - duty values below are 0-255

// Light fade time when changing brightness, in milliseconds (0 = no fade)
// Hardware fades are limited to about 10 seconds at 25kHz / 8-bit; longer values just fade faster
const int light_fade_ms = 3000;

// Pumping config
// The pump runs for pump_duration seconds at each time
int pump_times[] = {
    300, // 5:00
    305, // 5:05
    310, // 5:10
    420, // 7:00
    425, // 7:05
    430, // 7:10
    600, // 10:00
    605, // 10:05
    610, // 10:10
    780, // 13:00
    785, // 13:05
    790, // 13:10
    960, // 16:00
    965, // 16:05
    970, // 16:10
    1140, // 19:00
    1145, // 19:05
    1150, // 19:10
    1320, // 22:00
    1325, // 22:05
    1330, // 22:10
};
int pump_duration = 10; // in seconds

// Light config (max 100 entries)
// Each entry: { time, light, duty }
//   light: 0 = both lights, 1 = light 1 only, 2 = light 2 only
//   duty:  0 (off) to 255 (full brightness)
int light_times[][3] = {
    {360,  0, 128}, // 6:00  - both lights 50%
    {420,  0, 255}, // 7:00  - both lights 100%
    {1260, 0, 128}, // 21:00 - both lights 50%
    {1320, 0, 0},   // 22:00 - both lights off
};

// Fan config
// Each entry: { time, duty }
//   duty: 0 (off) to 255 (full speed)
int fan_times[][2] = {
    {360,  128}, // 6:00  - fan 50%
    {1320, 51},  // 22:00 - fan 20%
};
