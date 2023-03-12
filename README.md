
# What is it

I have a few home automation devices on my todo list like sensors, lights etc.
Usually I am using ESP32s connected to my MQTT broker for these things, so I decided to write a basic firmware with all the necessary features to create a starting point for developing the individual firmwares

# Used Versions etc

ESP-IDF: v5.0.1
Hardware: ESP32-WROOM-DA with 4MB

# Features

- System info on startup
- Wifi with settings from flash
- Time sync from NTP server
- MQTT

# Notes

- PSRAM is enabled, but ignored if not found
- The system stops at a panic and is not rebooting!

# TODOs

- JSON Rx/Tx
- Auto Update
- Remote FW update mit fallback
- Handling of WiFi disconnects and mqtt reconnect
- Error handling, not simple ESP_ERROR_CHECKs
- Wrapper for accessing NVS
- Namespacing of NVS Keys

# WONT DO

- Firmware signing
- Secured MQTT
- Configurable timezones
