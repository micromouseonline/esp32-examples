# Multi-target Code and Examples

This repository holds a number of separate, but related, Arduino projects for ESP32 boards used in the Mk2 UKMARS timing system.

Some projects are for direct use when testing the timing system components, others serve simply as examples of techniques used.

## Building the projects

All projects can be built using either VSCode+PlatformIO, or in the Arduino IDE V2.x.x. 
Each projects can be built for a variety of supported boards. Full instructions are in the file [BUILDING.md](Building.md)

---
## **PROJECTS**

- **blinky-freertos**
    Going beyond the standard blinky starter, this project shows how to create a task in freeRTOS that blinks two LEDs at different rates. 
    It is an artificial example intended just to demonstrate how it is possible to use a single task with multiple parameters

- **buttons-freertos**
    l'kj'l