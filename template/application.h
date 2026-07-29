#pragma once

#include <Arduino.h>

#ifndef PLATFORMIO
// Arduino IDE: uncomment ONE board. PlatformIO users: leave all commented.
#define BOARD_S3_ZERO
// #define BOARD_C3_SUPER_MINI
// #define BOARD_C3_XIAO
// #define BOARD_C6_SUPER_MINI
#endif

void app_setup();
void app_loop();
