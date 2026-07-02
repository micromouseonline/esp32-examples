/*
 * cerberus-gate-controller
 *
 * Template for a multi-target ESP32 Arduino project.
 *
 * SUPPORTED BOARDS
 *   BOARD_S3_ZERO       - ESP32-S3 Zero (RGB LED on pin 21)
 *   BOARD_C3_SUPER_MINI - ESP32-C3 Super Mini (LED on pin 8)
 *   BOARD_C3_XIAO       - Seeed XIAO ESP32-C3 (LED on pin 10)
 *   BOARD_C6_SUPER_MINI - ESP32-C6 Super Mini (LED on pin 15)
 *
 * ARDUINO IDE (2.x only)
 *   Uncomment exactly one BOARD_xxx define in src/board_config.h before building.
 *   Arduino IDE 1.x is not supported; it cannot resolve #include "src/...".
 *
 * PLATFORMIO
 *   Board selection is handled automatically via build_flags in platformio.ini.
 *   Leave all BOARD_xxx defines in board_config.h commented out.
 *
 * BEFORE DEPLOYMENT
 *   Change the AP password in src/network-manager.cpp (search for "password123").
 */
