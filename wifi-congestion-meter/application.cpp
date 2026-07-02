#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <lwip/errno.h>
#include <lwip/sockets.h>

#include "../common/board-config.h"
#include "../common/secrets.h"

// --- CONFIGURATION ---

const uint16_t port = 1234;
#define SAMPLE_PERIOD_MS 1000

#define RX_BUFFER_SIZE 1460
uint8_t rx_buffer[RX_BUFFER_SIZE];

volatile uint32_t total_packets = 0;
volatile uint32_t total_bytes = 0;
uint32_t last_execution_time = 0;

// High-speed receiver task running on Core 1
void vReceiverTask(void* pvParameters) {
  int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock < 0) {
    Serial.println("[ERROR] Failed to create socket.");
    vTaskDelete(NULL);
  }

  struct sockaddr_in bind_addr;
  bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // Listen to any incoming IP address
  bind_addr.sin_family = AF_INET;
  bind_addr.sin_port = htons(port);

  if (bind(sock, (struct sockaddr*)&bind_addr, sizeof(bind_addr)) < 0) {
    Serial.println("[ERROR] Failed to bind socket.");
    close(sock);
    vTaskDelete(NULL);
  }

  struct sockaddr_in source_addr;
  socklen_t socklen = sizeof(source_addr);

  Serial.printf("[SYSTEM] Listening for UDP frames on port %d...\n", port);

  while (1) {
    // recvfrom blocks efficiently until a packet arrives
    int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer), 0, (struct sockaddr*)&source_addr, &socklen);

    if (len > 0) {
      total_packets++;
      total_bytes += len;
    }
  }
}

void print_bar_chart(float utilization) {
  const int BAR_WIDTH = 20;
  int progress = (int)((utilization / 100.0) * BAR_WIDTH);
  if (progress > BAR_WIDTH)
    progress = BAR_WIDTH;
  if (progress < 0)
    progress = 0;

  Serial.print("[");
  for (int i = 0; i < BAR_WIDTH; i++) {
    if (i < progress)
      Serial.print("=");
    else
      Serial.print(" ");
  }
  Serial.print("] ");
}

void app_setup() {
  delay(2000);
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== NATIVE UDP PORT CONGESTION METER ===");

  // Connect to the Access Point
  Serial.printf("[SYSTEM] Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  WiFi.setTxPower(WIFI_POWER_11dBm);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Disable power saving so the receiver radio stays active constantly
  esp_wifi_set_ps(WIFI_PS_NONE);

  Serial.println("\n[SYSTEM] Connected!");
  Serial.printf("[SYSTEM] Listener IP Address: %s\n", WiFi.localIP().toString().c_str());
  Serial.println("[SYSTEM] Enter this IP into your blaster configurations.");

  // Launch the receiver worker task at Priority 2
  xTaskCreatePinnedToCore(vReceiverTask, "rx_task", 4096, NULL, 2, NULL, 1);

  last_execution_time = millis();
}

void app_loop() {
  uint32_t current_time = millis();

  if (current_time - last_execution_time >= SAMPLE_PERIOD_MS) {
    // Atomic snapshot of metrics
    noInterrupts();
    uint32_t pkts = total_packets;
    uint32_t bytes = total_bytes;
    total_packets = 0;
    total_bytes = 0;
    interrupts();

    last_execution_time = current_time;

    // Math calculation against real-world 2.4GHz capacity limit (~3.5 MB/s)
    float megabytes_per_second = ((float)bytes / 1024.0f) / 1024.0f;
    float utilization_pct = (megabytes_per_second / 3.5f) * 100.0f;
    if (utilization_pct > 100.0f)
      utilization_pct = 100.0f;

    // Display status
    Serial.printf("PORT %-5d ", port);
    print_bar_chart(utilization_pct);
    Serial.printf("| %5.1f%% Utilized | %5d Pkts/sec | %6.1f KB/sec\n", utilization_pct, pkts, (float)bytes / 1024.0f);
  }
}