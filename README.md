# Hydro

Firmware for an ESP32-based hydroponics controller. It runs a water pump on a timed schedule, controls a fan and two grow lights with PWM, monitors the water level, and shows the system status on a small OLED display.

## Features

- **Scheduled watering**: the pump runs for a set number of seconds at each configured time of day.
- **Fan control**: fan speed (PWM) changes at scheduled times.
- **Grow lights**: two independently dimmable lights (PWM), scheduled together or separately.
- **Water level monitoring**: the display shows `~ OK` or `~ LOW` from a digital level sensor.
- **OLED status display**: water level, light and fan levels, current time, and the next watering time.
- **Network time**: the clock is set from an NTP server at boot. WiFi is then turned off, and the ESP32's internal clock keeps time.
- **Recovery after a reboot**: on startup the fan and lights go straight to the level the schedule says they should be at.

## Hardware

| Component | Notes |
|---|---|
| Wemos D1 Mini32 (ESP32) | Other ESP32 boards work if you change `board` in `platformio.ini` |
| SSD1306 128×64 OLED, I2C | Address `0x3C`, on the default I2C pins: SDA = GPIO 21, SCL = GPIO 22 |
| Relay module | Switches the water pump |
| PWM fan | A 4-pin PC fan works well at the default 25 kHz |
| 2 × dimmable grow lights | Driven by PWM, e.g. through MOSFET drivers |
| Digital water level sensor | HIGH = OK, LOW = low water. Needs an external pull-up or pull-down resistor |

### Default pins

These are the defaults in `src/config.example.h`. Change them to match your wiring.

| Function | GPIO |
|---|---|
| Pump relay | 17 |
| Water level sensor | 27 |
| Fan PWM | 4 |
| Light 1 PWM | 25 |
| Light 2 PWM | 26 |

Avoid GPIO 21 and 22 (the display) and the ESP32 strapping pins 0, 2, 5, 12 and 15.

## Getting started

### 1. Install the tools

1. Install [Visual Studio Code](https://code.visualstudio.com/).
2. Install the **PlatformIO IDE** extension.
3. Open this folder in VS Code.

The project uses the community [pioarduino](https://github.com/pioarduino/platform-espressif32) platform, which provides Arduino-ESP32 3.x. You don't need to install it separately: PlatformIO downloads it, along with the libraries, on the first build.

### 2. Create your config

Copy the example config and edit it:

```sh
cp src/config.example.h src/config.h
```

At minimum, set:

- `wifi_ssid` and `wifi_password`
- `gmtOffset_sec` and `daylightOffset_sec` for your timezone
- the `PIN_*` values, to match your wiring

`src/config.h` is ignored by git, so your WiFi credentials stay out of the repository.

### 3. Build and upload

Connect the board over USB, then either:

- click **Upload** (→) in the PlatformIO toolbar in VS Code, or
- run the command:

  ```sh
  pio run -t upload
  ```

The first build takes several minutes while PlatformIO downloads the toolchain.

If the upload fails to connect, hold the **BOOT** button on the board while the upload starts. If the board doesn't show up as a port at all, you may need a USB driver: CH340 or CP210x, depending on the board.

### 4. Monitor

To see debug output, open the serial monitor at 115200 baud:

```sh
pio device monitor
```

## Configuration

All settings are in `src/config.h`.

### Schedule times

Schedule times are given as **minutes since midnight**:

| Time | Value |
|---|---|
| 05:00 | `5 * 60` = `300` |
| 14:30 | `14 * 60 + 30` = `870` |
| 22:00 | `22 * 60` = `1320` |

Don't use `0` (midnight) as a schedule time. It interferes with the daily timer reset.

### Pump

```cpp
int pump_times[] = { 300, 305, 310, 420 };  // 5:00, 5:05, 5:10, 7:00
int pump_duration = 10;                     // seconds per run
```

### Lights

Each entry is `{ time, light, duty }`:

- `light`: `0` = both lights, `1` = light 1, `2` = light 2
- `duty`: `0` (off) to `255` (full brightness)

```cpp
int light_times[][3] = {
    {360,  0, 128}, // 6:00  - both lights 50%
    {420,  0, 255}, // 7:00  - both lights 100%
    {1320, 0, 0},   // 22:00 - both lights off
};
```

Up to 100 entries.

### Fan

Each entry is `{ time, duty }`, where `duty` is `0` (off) to `255` (full speed):

```cpp
int fan_times[][2] = {
    {360,  128}, // 6:00  - fan 50%
    {1320, 51},  // 22:00 - fan 20%
};
```

### PWM

```cpp
const int pwm_frequency  = 25000; // Hz
const int pwm_resolution = 8;     // bits; gives the 0-255 duty range used above
```

If you change the resolution, the duty values in your schedules and the percentages on the display need to change to match.

## How it behaves

- **At boot:** the controller shows the logo, connects to WiFi, sets the clock from NTP, and disconnects. It then sets the fan and lights to the levels the schedule says they should be at for the current time.
- **During the day:** each pump, fan and light entry fires once, when its minute arrives.
- **At 00:01:** all schedule entries are reset for the new day.
- **Missed waterings are skipped.** If the board was off or rebooting at a watering time, it doesn't catch up. This stops a reboot from causing extra watering.

## Test sketches

The `tests/` folder holds standalone sketches for checking each piece of hardware:

| File | Tests |
|---|---|
| `tests/display.cpp` | OLED display (Adafruit demo) |
| `tests/pwmfan.cpp` | Fan PWM, ramping speed up and down |
| `tests/relay.cpp` | Relay, toggling every second |
| `tests/wifitime.cpp` | WiFi connection and NTP time |

To run one, temporarily move `src/main.cpp` out of `src/` and copy the test file in its place, then build and upload as usual. Put `main.cpp` back afterwards.

The pin numbers in the test sketches are hard-coded. Check them before running.

## Known limitations

- The clock is only synced at boot, so it slowly drifts over long uptimes, and daylight saving changes aren't applied automatically. Rebooting resyncs the clock.
- The pump uses a blocking delay. The display and the other timers pause while it runs.
- If NTP returns exactly 00:00, the controller treats the time as invalid and retries, so a boot at midnight may be delayed.

## Credits

- [ESP32Time](https://github.com/fbiego/ESP32Time) by fbiego, included in `lib/`
- [Adafruit SSD1306](https://github.com/adafruit/Adafruit_SSD1306) and [Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library)
