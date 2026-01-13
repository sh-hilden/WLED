# VL53L0X distance usermod

Reads distance from a VL53L0X time-of-flight sensor over I2C and publishes it via MQTT, similar to the temperature usermod.

## Installation

1. Attach VL53L0X sensor to the I2C pins for your board.
2. Add `-D USERMOD_VL53L0X_DISTANCE` to your build flags in `platformio.ini` (or `platformio_override.ini`).
3. Add the Pololu VL53L0X library to `lib_deps`:

```ini
lib_deps = ${env.lib_deps}
  pololu/VL53L0X @ ^1.3.0
```

Example `platformio_override.ini`:

```ini
[env:nodemcuv2]
build_flags = ${env.build_flags} -D USERMOD_VL53L0X_DISTANCE
lib_deps = ${env.lib_deps}
  pololu/VL53L0X @ ^1.3.0
```

## MQTT topics

When MQTT is enabled, the usermod publishes:

- `<mqttDeviceTopic>/distance` (millimeters)
- `<mqttDeviceTopic>/distance_cm` (centimeters, one decimal)

## Configuration (cfg.json)

Under `um`:

```json
"VL53L0XDistance": {
  "enabled": true,
  "read-interval-ms": 1000
}
```
