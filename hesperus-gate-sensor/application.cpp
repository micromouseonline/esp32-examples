#include "application.h"
#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "sensor.h"

#include "../common/board-config.h"
#include "../common/board-id.h"
#include "../common/esp32_info.h"
#include "../common/secrets.h"
#include "../common/wifi-manager.h"

StatusLED statusIndicator;

void app_setup() {
  statusIndicator.begin();

  delay(2000);
  Serial.begin(115200);
  statusIndicator.turnOn();
  delay(1000);
  digitalWrite(STATUS_LED, LOW);
  Serial.println(F("CERBERUS: gate controller"));

  wifi_connect(statusIndicator);
  sensor_init();
}

//----------------------------------------------------------------------------------------------

void app_loop() {
  statusIndicator.toggle();
  sensor_update();
  delay(200);
}
