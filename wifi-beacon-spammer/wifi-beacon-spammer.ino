#ifndef PLATFORMIO
// Arduino IDE: uncomment ONE board. PlatformIO users: leave all commented.
// #define BOARD_S3_ZERO
// #define BOARD_S3_SUPER_MINI
// #define BOARD_S3_CYD_TOUCH_FREENOVE
// #define BOARD_C3_SUPER_MINI
// #define BOARD_C3_XIAO
// #define BOARD_C6_XIAO
// #define BOARD_C6_SUPER_MINI
#endif

#include <Arduino.h>

#include "application.h"

void setup() {
  app_setup();
}

void loop() {
  app_loop();
}