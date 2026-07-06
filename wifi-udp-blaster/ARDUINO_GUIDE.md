# Arduino IDE Setup Reference Guide
> **Note:** This file is auto-generated from `platformio.ini` and `base-boards.ini`.

## Target Environment: `version_metadata`
### 1. Arduino IDE Menu Settings
* **Target Board:** Select the menu item corresponding to platformio board target: `Unknown`
* **Flash Size:** Default

---

## Target Environment: `wifi-udp-blaster-esp32-s3-zero`
### 1. Arduino IDE Menu Settings
* **Target Board:** Select the menu item corresponding to platformio board target: `esp32-s3-devkitc-1`
* **Flash Size:** 4MB

### 2. Required Compiler Macros
Create a file named `config_flags.h` in your sketch and paste:
```cpp
#define HAS_NEOPIXEL 1
#define ARDUINO_USB_DFU_ON_BOOT 0
#define ARDUINO_USB_CDC_ON_BOOT 1
#define ARDUINO_USB_MODE 1
#define BOARD_HAS_PSRAM 1
#define BOARD_S3_ZERO 1
#define STATUS_LED 21
#define NEOPIXEL_COLOR_ORDER NEO_RGB
```

### 3. Required Libraries (Install via Library Manager)
* adafruit/Adafruit NeoPixel @ ^1.15.5
* Networking

---

## Target Environment: `wifi-udp-blaster-esp32-s3-super-mini`
### 1. Arduino IDE Menu Settings
* **Target Board:** Select the menu item corresponding to platformio board target: `esp32-s3-devkitc-1`
* **Flash Size:** 4MB

### 2. Required Compiler Macros
Create a file named `config_flags.h` in your sketch and paste:
```cpp
#define HAS_NEOPIXEL 1
#define ARDUINO_USB_DFU_ON_BOOT 0
#define ARDUINO_USB_CDC_ON_BOOT 1
#define ARDUINO_USB_MODE 1
#define BOARD_HAS_PSRAM 1
#define BOARD_S3_SUPER_MINI 1
#define STATUS_LED 48
#define NEOPIXEL_COLOR_ORDER NEO_GRB
```

### 3. Required Libraries (Install via Library Manager)
* adafruit/Adafruit NeoPixel @ ^1.15.5
* Networking

---

## Target Environment: `wifi-udp-blaster-esp32-s3-cyd-touch-freenove`
### 1. Arduino IDE Menu Settings
* **Target Board:** Select the menu item corresponding to platformio board target: `esp32-s3-devkitc-1`
* **Flash Size:** 16MB

### 2. Required Compiler Macros
Create a file named `config_flags.h` in your sketch and paste:
```cpp
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
* olikraus/U8g2 @ ^2.36.2
* adafruit/Adafruit NeoPixel @ ^1.15.5
* Networking

---

## Target Environment: `wifi-udp-blaster-esp32-c3`
### 1. Arduino IDE Menu Settings
* **Target Board:** Select the menu item corresponding to platformio board target: `esp32-c3-devkitm-1`
* **Flash Size:** Default

### 2. Required Compiler Macros
Create a file named `config_flags.h` in your sketch and paste:
```cpp
#define ARDUINO_USB_CDC_ON_BOOT 1
#define ARDUINO_USB_MODE 1
#define BOARD_C3_SUPER_MINI 1
#define HAS_LED 1
#define STATUS_LED 8
#define LED_ACTIVE_LOW 1
```

### 3. Required Libraries (Install via Library Manager)
* Networking

---

## Target Environment: `wifi-udp-blaster-esp32-c3-xiao`
### 1. Arduino IDE Menu Settings
* **Target Board:** Select the menu item corresponding to platformio board target: `seeed_xiao_esp32c3`
* **Flash Size:** Default

### 2. Required Compiler Macros
Create a file named `config_flags.h` in your sketch and paste:
```cpp
#define ARDUINO_USB_CDC_ON_BOOT 1
#define ARDUINO_USB_MODE 1
#define BOARD_C3_XIAO 1
#define HAS_LED 1
#define STATUS_LED 10
```

### 3. Required Libraries (Install via Library Manager)
* Networking

---

## Target Environment: `wifi-udp-blaster-esp32-c6-xiao`
### 1. Arduino IDE Menu Settings
* **Target Board:** Select the menu item corresponding to platformio board target: `esp32-c6-devkitc-1`
* **Flash Size:** 8MB

### 2. Required Compiler Macros
Create a file named `config_flags.h` in your sketch and paste:
```cpp
#define ARDUINO_USB_CDC_ON_BOOT 1
#define ARDUINO_USB_MODE 1
#define BOARD_C6_XIAO 1
#define HAS_LED 1
#define STATUS_LED 15
#define LED_ACTIVE_LOW 1
```

### 3. Required Libraries (Install via Library Manager)
* Networking

---

## Target Environment: `wifi-udp-blaster-esp32-c6`
### 1. Arduino IDE Menu Settings
* **Target Board:** Select the menu item corresponding to platformio board target: `esp32-c6-devkitc-1`
* **Flash Size:** 4MB

### 2. Required Compiler Macros
Create a file named `config_flags.h` in your sketch and paste:
```cpp
#define HAS_NEOPIXEL 1
#define ARDUINO_USB_CDC_ON_BOOT 1
#define ARDUINO_USB_MODE 1
#define ARDUINO_USB_MSC_ON_BOOT 0
#define BOARD_C6_SUPER_MINI 1
#define STATUS_LED 15
#define NEOPIXEL_COLOR_ORDER NEO_RGB
```

### 3. Required Libraries (Install via Library Manager)
* adafruit/Adafruit NeoPixel @ ^1.15.5
* Networking

---

