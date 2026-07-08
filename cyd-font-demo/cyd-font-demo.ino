#ifndef PLATFORMIO
// Arduino IDE: uncomment ONE board. PlatformIO users: leave all commented.
// #define BOARD_CYD2USB_DIYMALLS_ILI9341
// #define BOARD_CYD2USB_DIYMALLS_ST7789
// #define BOARD_JC2432W328C
// #define BOARD_S3_CYD_TOUCH_FREENOVE
#endif

#include <Arduino.h>

#include "application.h"

void setup() {
  app_setup();
}

void loop() {
  app_loop();
}
