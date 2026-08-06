/// @file application.cpp
/// @brief Raw 802.11 beacon flood for WiFi resilience stress testing.
///
/// Transmits real 2.4GHz RF at high packet rate. Only use against your own
/// equipment, on spectrum/premises you are authorized to test on -- never
/// against third-party networks or in shared/public airspace.

#include <cctype>

#include <Arduino.h>
#include <WiFi.h>
#include <esp_system.h>
#include <esp_wifi.h>

#include "../common/board-config.h"
#include "../common/button.h"
#include "../common/secrets.h"
#include "../common/status-led.h"
#include "beacon-frame.h"

#define BEACON_POOL_SIZE 32
#define BUTTON_PIN 0  // onboard boot button, pulls low when pressed

// Capped well below max (~19.5dBm) to keep sustained-TX current draw from tripping brownout
// on marginal USB power. Tune upward only if fed from a supply that can take it -- this tool
// is meant for close-range use next to the equipment under test, not range.
#define BEACON_TX_POWER WIFI_POWER_8_5dBm

// Channels away from the auto-detected target network's channel to actually transmit on.
// 0 = co-channel (default; max airtime contention via shared CSMA/CA with the target).
// Nonzero tests adjacent/nearby-channel interference instead -- still disruptive (2.4GHz
// channels are 5MHz apart but ~20-22MHz wide, so anything within +/-4 still overlaps
// heavily), but via raw noise/corruption rather than polite airtime-sharing, since the
// target's radio won't defer to a transmitter it doesn't recognize as sharing its channel.
#define BEACON_CHANNEL_OFFSET 0

static uint8_t poolBuf[BEACON_POOL_SIZE][beacon_frame::kMaxFrameLen];
static size_t poolLen[BEACON_POOL_SIZE];
static uint8_t poolMac[BEACON_POOL_SIZE][6];
static char poolSsid[BEACON_POOL_SIZE][beacon_frame::kMaxSsidLen + 1];

static volatile uint8_t currentChannel = 0;
static volatile uint32_t framesSent = 0;
static volatile uint32_t bytesSent = 0;
static volatile bool spamming = false;
static uint32_t lastStatsTime = 0;

static StatusLED led;
static DebouncedButton button(BUTTON_PIN, /*activeLow=*/true);

/// Non-blocking line accumulator: returns true once a full line has been read into lineBuf.
/// Shared by the boot-time manual-channel prompt and the runtime Serial override.
bool pollSerialLine(char* lineBuf, size_t lineBufSize) {
  static size_t idx = 0;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (idx == 0) {
        continue;
      }
      lineBuf[idx] = '\0';
      idx = 0;
      return true;
    }
    if (idx < lineBufSize - 1) {
      lineBuf[idx++] = c;
    }
  }
  return false;
}

/// Scans for targetSsid, printing the same SSID/RSSI/channel/encryption table as before.
/// Returns the channel targetSsid was found on, or -1 if not found.
int findTargetChannel(const char* targetSsid) {
  Serial.println("Scanning for target network...");
  int n = WiFi.scanNetworks();
  int found = -1;
  if (n == 0) {
    Serial.println("no networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    Serial.println("Nr | SSID                             | RSSI | CH | Encryption");
    for (int i = 0; i < n; ++i) {
      Serial.printf("%2d", i + 1);
      Serial.print(" | ");
      Serial.printf("%-32.32s", WiFi.SSID(i).c_str());
      Serial.print(" | ");
      Serial.printf("%4ld", WiFi.RSSI(i));
      Serial.print(" | ");
      Serial.printf("%2ld", WiFi.channel(i));
      Serial.print(" | ");
      switch (WiFi.encryptionType(i)) {
        case WIFI_AUTH_OPEN:
          Serial.print("open");
          break;
        case WIFI_AUTH_WEP:
          Serial.print("WEP");
          break;
        case WIFI_AUTH_WPA_PSK:
          Serial.print("WPA");
          break;
        case WIFI_AUTH_WPA2_PSK:
          Serial.print("WPA2");
          break;
        case WIFI_AUTH_WPA_WPA2_PSK:
          Serial.print("WPA+WPA2");
          break;
        case WIFI_AUTH_WPA2_ENTERPRISE:
          Serial.print("WPA2-EAP");
          break;
        case WIFI_AUTH_WPA3_PSK:
          Serial.print("WPA3");
          break;
        case WIFI_AUTH_WPA2_WPA3_PSK:
          Serial.print("WPA2+WPA3");
          break;
        case WIFI_AUTH_WAPI_PSK:
          Serial.print("WAPI");
          break;
        default:
          Serial.print("unknown");
      }
      Serial.println();
      if (found < 0 && WiFi.SSID(i) == targetSsid) {
        found = WiFi.channel(i);
      }
      delay(10);
    }
  }
  Serial.println("");
  WiFi.scanDelete();
  return found;
}

/// Blocks, re-prompting on invalid input, until a valid channel (1-13) is entered over Serial.
int promptManualChannel() {
  Serial.println("Target SSID not found in scan.");
  char line[16];
  while (true) {
    Serial.println("Enter channel number (1-13):");
    while (!pollSerialLine(line, sizeof(line))) {
      delay(10);
    }
    int ch = atoi(line);
    if (ch >= 1 && ch <= 13) {
      return ch;
    }
    Serial.println("Invalid channel, try again.");
  }
}

/// Generates the fake-AP pool (random MAC + derived SSID per slot) and builds each frame
/// for the given channel. MACs/SSIDs are stable for the run; only called once at startup.
void buildPool(uint8_t channel) {
  for (int i = 0; i < BEACON_POOL_SIZE; ++i) {
    beacon_frame::randomLocallyAdministeredMac(poolMac[i]);
    snprintf(poolSsid[i], sizeof(poolSsid[i]), "KRONOS-STRESSTEST-%02X%02X", poolMac[i][4], poolMac[i][5]);
    poolLen[i] = beacon_frame::buildBeaconFrame(poolBuf[i], poolMac[i], poolSsid[i], channel);
  }
}

/// Points the radio at channel and patches every pool frame's DS Parameter Set byte to match.
void setTargetChannel(uint8_t channel) {
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  for (int i = 0; i < BEACON_POOL_SIZE; ++i) {
    size_t ssidLen = strlen(poolSsid[i]);
    poolBuf[i][beacon_frame::dsParamChannelOffset(ssidLen)] = channel;
  }
  currentChannel = channel;
}

void vBeaconFloodTask(void* pvParameters) {
  while (true) {
    if (spamming) {
      for (int i = 0; i < BEACON_POOL_SIZE; ++i) {
        esp_err_t err = esp_wifi_80211_tx(WIFI_IF_STA, poolBuf[i], poolLen[i], true);
        if (err == ESP_OK) {
          framesSent++;
          bytesSent += poolLen[i];
        }
      }
      // Feed the watchdog / yield to the WiFi driver once per full pool pass, not per-frame,
      // so transmission stays back-to-back within a pass.
      vTaskDelay(1);
    } else {
      vTaskDelay(50);
    }
  }
}

void app_setup() {
  led.begin();
  led.setRGB(0, 0, 32);  // booting/searching = blue
  button.begin();

  // Don't block on a host serial monitor being attached -- this device needs to run
  // standalone in the field, not just tethered to a PC.
  Serial.begin(115200);
  delay(1000);

  Serial.println("[SYSTEM] wifi-beacon-spammer starting");
  Serial.println(
      "[WARN] Transmits real 2.4GHz RF at high packet rate. Only use against your own "
      "equipment on spectrum you are authorized to test on.");

  // esp_reset_reason() reads RTC-retained state, so it survives the native USB CDC
  // reconnect gap that otherwise swallows any panic/brownout message printed at reset.
  const char* resetReasonStr = "UNKNOWN";
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON:
      resetReasonStr = "POWERON";
      break;
    case ESP_RST_SW:
      resetReasonStr = "SW (esp_restart)";
      break;
    case ESP_RST_PANIC:
      resetReasonStr = "PANIC (crash)";
      break;
    case ESP_RST_INT_WDT:
      resetReasonStr = "INT_WDT";
      break;
    case ESP_RST_TASK_WDT:
      resetReasonStr = "TASK_WDT";
      break;
    case ESP_RST_WDT:
      resetReasonStr = "OTHER_WDT";
      break;
    case ESP_RST_BROWNOUT:
      resetReasonStr = "BROWNOUT";
      break;
    default:
      break;
  }
  Serial.printf("[SYSTEM] Last reset reason: %s\n", resetReasonStr);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // esp_wifi_start() (triggered synchronously by WiFi.mode()) has already completed here,
  // but Arduino's own "STA started" bookkeeping flag only flips once the async STA_START
  // event is dispatched -- WiFi.setTxPower()/getTxPower() check that flag and silently
  // no-op if called too early. Call the raw esp_wifi_* functions instead to avoid the race.
  esp_wifi_set_max_tx_power((int8_t)BEACON_TX_POWER);
  int8_t actualTxPower = 0;
  esp_wifi_get_max_tx_power(&actualTxPower);
  Serial.printf("[SYSTEM] TX power capped to %.2f dBm\n", actualTxPower * 0.25f);

  int targetChannel = findTargetChannel(ssid);
  if (targetChannel < 0) {
    targetChannel = promptManualChannel();
  } else {
    Serial.printf("[SYSTEM] Found target SSID '%s' on channel %d\n", ssid, targetChannel);
  }

  int txChannel = targetChannel + BEACON_CHANNEL_OFFSET;
  if (txChannel < 1) {
    txChannel = 1;
  } else if (txChannel > 13) {
    txChannel = 13;
  }
  if (BEACON_CHANNEL_OFFSET != 0) {
    Serial.printf("[SYSTEM] Transmitting on channel %d (target channel %d, offset %+d)\n", txChannel, targetChannel,
                  BEACON_CHANNEL_OFFSET);
  }

  buildPool((uint8_t)txChannel);
  setTargetChannel((uint8_t)txChannel);

  xTaskCreatePinnedToCore(vBeaconFloodTask, "beacon_flood", 4096, NULL, 1, NULL, 1);

  led.setRGB(0, 32, 0);  // passive/ready = green
  Serial.println("[SYSTEM] Passive. Press the button to start spamming; press again to stop.");
}

void app_loop() {
  if (button.wasPressed()) {
    spamming = !spamming;
    if (spamming) {
      led.setRGB(32, 0, 0);  // spamming = red
      noInterrupts();
      framesSent = 0;
      bytesSent = 0;
      interrupts();
      lastStatsTime = millis();
      Serial.println("[STATE] SPAMMING");
    } else {
      led.setRGB(0, 32, 0);  // passive = green
      Serial.println("[STATE] PASSIVE");
    }
  }

  char line[16];
  if (pollSerialLine(line, sizeof(line))) {
    size_t len = strlen(line);
    bool allDigits = len > 0;
    for (size_t i = 0; i < len; ++i) {
      if (!isdigit((unsigned char)line[i])) {
        allDigits = false;
        break;
      }
    }
    if (allDigits) {
      int ch = atoi(line);
      if (ch >= 1 && ch <= 13) {
        setTargetChannel((uint8_t)ch);
        Serial.printf("[CHANNEL] override -> %d\n", ch);
      } else {
        Serial.println("[CHANNEL] invalid channel, must be 1-13");
      }
    }
  }

  if (spamming) {
    uint32_t now = millis();
    if (now - lastStatsTime >= 1000) {
      lastStatsTime = now;

      noInterrupts();
      uint32_t frames = framesSent;
      uint32_t bytes = bytesSent;
      framesSent = 0;
      bytesSent = 0;
      interrupts();

      float mb = (float)bytes / 1024.0f / 1024.0f;
      Serial.printf("[STATISTICS] %lu beacons/s | %.2f MB/s | channel %u | pool %d APs\n", (unsigned long)frames, mb,
                    currentChannel, BEACON_POOL_SIZE);
    }
  }
}
