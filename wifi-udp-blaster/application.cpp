#include <WiFi.h>
#include <esp_wifi.h>

#include "../common/board-config.h"
#include "../common/esp32_info.h"
#include "../common/secrets.h"

#include <lwip/errno.h>
#include <lwip/sockets.h>

const uint16_t port = 1234;

// 802.11 data frame payload size (header is 24 bytes; total frame ~1500 bytes)
#define PACKET_SIZE 1460
uint8_t dummy_payload[PACKET_SIZE];
volatile uint32_t packets_sent = 0;

uint32_t last_display_time = 0;
void vHighSpeedBlasterTask(void* pvParameters) {
  // 1. Create a raw native UDP socket (bypasses Arduino WiFiUDP entirely)
  int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock < 0) {
    vTaskDelete(NULL);
  }

  // 2. Target a REAL, LIVE device on your network (like your laptop)
  // This allows the ESP32 to successfully resolve the ARP MAC address instantly!
  struct sockaddr_in dest_addr;
  dest_addr.sin_addr.s_addr = inet_addr("192.168.0.1");  // <-- MUST BE A LIVE IP
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_port = htons(port);

  Serial.println("[SYSTEM] Native firehose active.");

  while (1) {
    // sendto() drops the payload directly into the hardware Wi-Fi DMA queues
    int err = sendto(sock, dummy_payload, PACKET_SIZE, 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));

    if (err >= 0) {
      packets_sent = packets_sent + 1;
    } else {
      int tx_error = errno;
      if (tx_error == 12) {  // ENOMEM: Hardware queue is completely full
        // Yield just long enough for the radio to push a packet out the antenna
        vTaskDelay(1 / portTICK_PERIOD_MS);
      }
    }
  }
}

void app_setup() {
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, HIGH);
  delay(2000);
  Serial.begin(115200);
  delay(1000);
  digitalWrite(STATUS_LED, LOW);

  Serial.printf("\n[SYSTEM] Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  WiFi.setTxPower(WIFI_POWER_11dBm);

  int led_state = 1;
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    led_state = 1 - led_state;
    digitalWrite(STATUS_LED, led_state);
    Serial.print(".");
  }
  digitalWrite(STATUS_LED, LOW);
  esp_wifi_set_ps(WIFI_PS_NONE);

  Serial.println("\n[SYSTEM] Wi-Fi Connected!");
  Serial.printf("[SYSTEM] IP: %s  RSSI: %d dBm\n", WiFi.localIP().toString().c_str(), WiFi.RSSI());

  xTaskCreatePinnedToCore(vHighSpeedBlasterTask, "blaster", 4096, NULL, 1, NULL, 1);
}

void app_loop() {
  uint32_t current_time = millis();
  if (current_time - last_display_time >= 1000) {
    static int led_state = 1;
    led_state = 1 - led_state;
    digitalWrite(STATUS_LED, led_state);

    noInterrupts();
    uint32_t count = packets_sent;
    packets_sent = 0;
    interrupts();

    last_display_time = current_time;

    float mb = ((float)count * sizeof(dummy_payload)) / 1024.0f / 1024.0f;
    Serial.printf("[STATISTICS] %lu pkt/s | %.2f MB/s (%.1f Mbps) | RSSI %d dBm\n", count, mb, mb * 8.0f, WiFi.RSSI());
  }
}
