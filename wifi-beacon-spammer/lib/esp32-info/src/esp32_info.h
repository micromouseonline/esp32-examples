#pragma once

#include <Arduino.h>

/**
 * @brief Print the factory-programmed eFuse MAC address to Serial.
 *
 * Reads the 48-bit (or 64-bit on IEEE 802.15.4 variants) MAC address
 * from eFuse and prints it as colon-separated hex octets.
 */
void printFactoryMac(void);

/**
 * @brief Print a human-readable label for a flash chip mode to Serial.
 *
 * @param mode  Flash mode value returned by ESP.getFlashChipMode().
 */
void printFlashChipMode(FlashMode_t mode);

/**
 * @brief Print a full ESP32 system information report to Serial.
 *
 * Reports chip model, revision, core count, CPU frequency, SDK version,
 * flash size/speed/mode, PSRAM size, heap statistics, and sketch size.
 */
void getInfo(void);
