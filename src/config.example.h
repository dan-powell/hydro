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
    450, // 7:30
    660, // 11:00
    900, // 13:00 
    1020, // 17:00  
    1290, // 21:30
    1315, // 21:55
    1335, // 22:15
    1345 // 22:25
};
int pump_duration = 10; // in seconds