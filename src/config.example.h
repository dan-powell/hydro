// Wifi Settings
const char* wifi_ssid       = "";
const char* wifi_password   = "";

// Time settings
const char* ntpServer = "pool.ntp.org"; // NTP server to use
const long  gmtOffset_sec = 0; // Timezone offset
const int   daylightOffset_sec = 3600; // Daylight Savings Offset

// Pumping config
// Defined in minutes since 0:00
// e.g 11:00 -> 11 * 60 = 660
// e.g 14:30 -> 14 * 60 + 30 = 870 
// FIX - Can't currently set to 0 as that will cause infinite loops
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
int pump_duration = 3; // in seconds