## Useful signal strength vs quality of connection

| RSSI Range (dBm)   | Connection Quality       | What it means for the ESP32                                                                                                          |
| :----------------- | :----------------------- | :----------------------------------------------------------------------------------------------------------------------------------- |
| **-30 to -50 dBm** | **Amazing / Very Good** | The ESP32 is likely in the same room as the router. Max data throughput, near-zero packet loss, and blazing-fast connection times.    |
| **-50 to -65 dBm** | **Good** | A solid, dependable connection. Perfectly fine for heavy data transfers, OTA (Over-The-Air) firmware updates, and continuous streaming. |
| **-65 to -75 dBm** | **Adequate / Fair** | The baseline for a reliable connection. Fine for sending periodic sensor data (MQTT/HTTP), but you might experience occasional minor latency. |
| **-75 to -85 dBm** | **Poor** | The danger zone. The ESP32 will suffer from frequent packet drops, slow HTTP handshake times, and a higher power draw as it struggles. |
| **-85 dBm & lower**| **Marginal / Disconnected**| The absolute limit of the ESP32's internal antenna. Frequent disconnections, failure to obtain an IP address, or total inability.  |

