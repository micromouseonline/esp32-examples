# Multi-target Code and Examples

This repository holds a number of separate, but related, Arduino projects for ESP32 boards used in the Mk2 UKMARS timing system.

Some projects are for direct use when testing the timing system components, others serve simply as examples of techniques used.

## Building the projects

All projects can be built using either VSCode+PlatformIO, or in the Arduino IDE V2.x.x. 
Each projects can be built for a variety of supported boards. Full instructions are in the file [BUILDING.md](Building.md)

---
## **PROJECTS**

*(placeholder descriptions -- to be reviewed/expanded)*

- **blinky-freertos**
    Minimal FreeRTOS example: one task, reused with different parameters, blinking two
    LEDs at different rates.

- **ble-serial**
    BLE peripheral (Nordic UART Service) that streams a fake sensor reading to a
    connected phone/app, alongside two independently-blinking status LEDs.

- **cerberus-gate-controller**
    Touchscreen control panel (Freenove CYD or M5Stack Core) for pairing with and
    configuring a BLE-connected e-bike controller: speed limit, sleep timeout,
    brightness.

- **event-capture-freertos**
    Precision event-timing pipeline: ISR-timestamped inputs, debounced and
    time-corrected through a chain of FreeRTOS tasks, with optional WiFi time
    anchoring.

- **hesperus-gate-sensor**
    Skeleton gate-sensor node. WiFi connection and status LED scaffolding are in
    place; the actual sensor read/update logic is still a stub.

- **oled-display**
    Bench-test rig for cycling through fonts and OLED module variants (SSD1306,
    SH1106, several resolutions) using two buttons.

- **wifi-beacon-spammer**
    Placeholder -- currently just a WiFi network scanner; intended to eventually
    broadcast fake beacon frames, not implemented yet.

- **wifi-congestion-meter**
    Receives UDP traffic and prints a live bar-chart of channel utilization/throughput
    to serial. Pairs with `wifi-udp-blaster` as the sender.

- **wifi-scanner**
    Scans for nearby WiFi networks and prints SSID/RSSI/channel/encryption over
    serial.

- **wifi-udp-blaster**
    Sends UDP packets at maximum rate to a fixed target address, for network
    load/stress testing.