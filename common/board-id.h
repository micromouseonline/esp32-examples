#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <esp_mac.h>

#include "status-led.h"

#include <stdint.h>
#include <stdio.h>
#include "esp_mac.h"  // Modern ESP-IDF header (esp_efuse_mac_get_default is deprecated in IDF v5+)

struct BoardInfo {
  uint32_t id32;
  const char *name;
};

// Board IDs derived from the last 4 bytes of the MAC address
constexpr BoardInfo boards[] = {
    {0x0E209D7C, "GATE_02"},     // 18:8B:0E:20:9D:7C -> 192.168.0.81
    {0x0E22B6A4, "GATE_01"},     // 18:8B:0E:22:B6:A4 -> 192.168.0.80
    {0x45B077E0, "GATE_04"},     //
    {0x6ECC52D4, "GREEN-05"},    // AC:27:6E:CC:52:D4
    {0x6ECC5904, "GREEN-01"},    // AC:27:6E:CC:59:04
    {0x6ECC9A5C, "GREEN-06"},    // AC:27:6E:CC:9A:5C
    {0x85677860, "GREEN-08"},    // 28:84:85:67:78:60
    {0x8567826C, "GREEN-04"},    // 28:84:85:67:82:6C
    {0x8567D29C, "GREEN-17"},    // 28:84:85:67:D2:9C
    {0x8567EA68, "GREEN-03"},    // 28:84:85:67:EA:68
    {0x9037CC98, "GATE_03"},     //
    {0xAB6E5000, "M5STACK-01"},  // 98:F4:AB:6E:50:00
    {0xE6FFFE1E, "GATE_05"},     // AC:EB:E6:FF:FE:1E
};

inline uint32_t get_chip_id() {
  uint8_t mac[6];
  // Reads base MAC directly from eFuse factory registers without needing WiFi driver startup
  esp_efuse_mac_get_default(mac);

  return ((uint32_t)mac[2] << 24) | ((uint32_t)mac[3] << 16) | ((uint32_t)mac[4] << 8) | ((uint32_t)mac[5]);
}
/**
 * Returns the board name if found in the list.
 * If not found, populates `out_buf` (must be at least 11 bytes) with 0xXXXXXXXX.
 *
 * @param out_buf Buffer to store fallback string if missing from array.
 * @return const char* Pointer to the board name or `out_buf`.
 */
inline const char *get_board_name(char out_buf[11]) {
  uint32_t id32 = get_chip_id();

  for (const auto &b : boards) {
    if (id32 == b.id32) {
      return b.name;
    }
  }

  // Safe fallback: Caller manages memory via out_buf
  snprintf(out_buf, 11, "0x%08X", (unsigned int)id32);
  return out_buf;
}