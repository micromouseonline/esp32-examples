# Arduino IDE Setup Reference Guide
> **Note:** This file is auto-generated from `platformio.ini` and `base-boards.ini`.

## Target Environment: `version_metadata`
### 1. Arduino IDE Menu Settings
* **Target Board:** Select the menu item corresponding to platformio board target: `Unknown`
* **Flash Size:** Default

---

## Target Environment: `cerberus-esp32-s3-cyd-touch-freenove`
### 1. Arduino IDE Menu Settings
* **Target Board:** Select the menu item corresponding to platformio board target: `esp32-s3-devkitc-1`
* **Flash Size:** 16MB

### 2. Required Compiler Macros
Create a file named `config_flags.h` in your sketch and paste:
```cpp
#define HAS_NEOKEY_LIB 1
#define HAS_LOVYANGFX 1
#define HAS_DISPLAY 1
#define HAS_NEOPIXEL 1
#define ARDUINO_USB_DFU_ON_BOOT 0
#define ARDUINO_USB_CDC_ON_BOOT 1
#define ARDUINO_USB_MODE 1
#define BOARD_HAS_PSRAM 1
#define BOARD_S3_CYD_TOUCH_FREENOVE 1
#define STATUS_LED 42
```

### 3. Required Libraries (Install via Library Manager)
* adafruit/Adafruit BusIO @ ^1.17.2
* adafruit/Adafruit seesaw Library @ ^1.7.9
* lovyan03/LovyanGFX @ ^1.1.16
* olikraus/U8g2 @ ^2.36.2
* adafruit/Adafruit NeoPixel @ ^1.15.5
* Networking

---

## Target Environment: `cerberus-m5-core`
### 1. Arduino IDE Menu Settings
* **Target Board:** Select the menu item corresponding to platformio board target: `m5stack-core-esp32-16M`
* **Flash Size:** Default

### 2. Required Compiler Macros
Create a file named `config_flags.h` in your sketch and paste:
```cpp
#define HAS_NEOKEY_LIB 1
#define HAS_LOVYANGFX 1
#define HAS_DISPLAY 1
#define ARDUINO_USB_MODE 1
#define BOARD_M5_CORE 1
#define STATUS_LED -1
```

### 3. Required Libraries (Install via Library Manager)
* adafruit/Adafruit BusIO @ ^1.17.2
* adafruit/Adafruit seesaw Library @ ^1.7.9
* lovyan03/LovyanGFX @ ^1.1.16
* olikraus/U8g2 @ ^2.36.2
* Networking

---

## Target Environment: `cerberus-cyd2usb-diymalls-ili9341`
### 1. Arduino IDE Menu Settings
* **Target Board:** Select the menu item corresponding to platformio board target: `esp32dev`
* **Flash Size:** 4MB

### 2. Required Compiler Macros
Create a file named `config_flags.h` in your sketch and paste:
```cpp
#define HAS_NEOKEY_LIB 1
#define HAS_LOVYANGFX 1
#define HAS_DISPLAY 1
#define ARDUINO_USB_MODE 1
#define BOARD_CYD2USB_DIYMALLS_ILI9341 1
#define STATUS_LED -1
```

### 3. Required Libraries (Install via Library Manager)
* adafruit/Adafruit BusIO @ ^1.17.2
* adafruit/Adafruit seesaw Library @ ^1.7.9
* lovyan03/LovyanGFX @ ^1.1.16
* olikraus/U8g2 @ ^2.36.2
* Networking

---

## Target Environment: `cerberus-cyd2usb-diymalls-st7789`
### 1. Arduino IDE Menu Settings
* **Target Board:** Select the menu item corresponding to platformio board target: `esp32dev`
* **Flash Size:** 4MB

### 2. Required Compiler Macros
Create a file named `config_flags.h` in your sketch and paste:
```cpp
#define HAS_NEOKEY_LIB 1
#define HAS_LOVYANGFX 1
#define HAS_DISPLAY 1
#define ARDUINO_USB_MODE 1
#define BOARD_CYD2USB_DIYMALLS_ST7789 1
#define STATUS_LED -1
```

### 3. Required Libraries (Install via Library Manager)
* adafruit/Adafruit BusIO @ ^1.17.2
* adafruit/Adafruit seesaw Library @ ^1.7.9
* lovyan03/LovyanGFX @ ^1.1.16
* olikraus/U8g2 @ ^2.36.2
* Networking

---

## Target Environment: `cerberus-jc2432w328c`
### 1. Arduino IDE Menu Settings
* **Target Board:** Select the menu item corresponding to platformio board target: `esp32dev`
* **Flash Size:** 4MB

### 2. Required Compiler Macros
Create a file named `config_flags.h` in your sketch and paste:
```cpp
#define HAS_NEOKEY_LIB 1
#define HAS_LOVYANGFX 1
#define HAS_DISPLAY 1
#define ARDUINO_USB_MODE 1
#define BOARD_JC2432W328C 1
#define STATUS_LED 16
#define LED_ACTIVE_LOW 1
```

### 3. Required Libraries (Install via Library Manager)
* adafruit/Adafruit BusIO @ ^1.17.2
* adafruit/Adafruit seesaw Library @ ^1.7.9
* lovyan03/LovyanGFX @ ^1.1.16
* olikraus/U8g2 @ ^2.36.2
* Networking

---

