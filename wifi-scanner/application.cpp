#include <Arduino.h>
#include <WiFi.h>

#include "../common/board-config.h"
#include "../common/board-id.h"
#include "../common/esp32_info.h"
#include "../common/secrets.h"
#include "../common/wifi-manager.h"

StatusLED statusIndicator;

void scan() {
  Serial.println("Scan start");

  // WiFi.scanNetworks will return the number of networks found.
  statusIndicator.turnOn();
  int n = WiFi.scanNetworks();
  statusIndicator.turnOff();
  Serial.println("Scan done");
  if (n == 0) {
    Serial.println("no networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    Serial.println("Nr | SSID                             | RSSI | CH | Encryption");
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
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
      delay(10);
    }
  }
  Serial.println("");
  // Delete the scan result to free memory for code below.
  WiFi.scanDelete();
}

void app_setup() {
  statusIndicator.begin();
  Serial.begin(SERIAL_BAUD);
  uint32_t start_time = millis();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // it may take anything up to 2000ms altogether to get  a serial connection
  while (!Serial && (millis() < 2000)) {
    delay(10);
  }
  uint32_t ready_time = millis();
  // just because the hardware is ready, does not mean the terminal is ready
  // so allow time for that as well
  delay(500);
  statusIndicator.turnOn();
  Serial.printf("Board: %s\n", get_board_name());
  Serial.printf("System running after %lu ms.", millis());

  Serial.println("System running.");
}

void app_loop() {
  scan();
  vTaskDelay(pdMS_TO_TICKS(5000));
}
