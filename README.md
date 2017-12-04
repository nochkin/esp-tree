# esp-tree
Driving Christmas tree with WS2812 LED strip using ESP8266.
It uses esp-open-rtos as the primary target with ws2812_i2s and libesphttpd (for OTA updates) libraries.

# How to build
Install and configure esp-open-rtos.

Download or clone the repo to your local drive.

Run "make" to build the firmware.

Run "make flash" to flash it to your ESP8266 module over serial using esptool.

