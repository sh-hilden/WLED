#pragma once

#include "wled.h"

#include <Wire.h>
#include <VL53L0X.h>

// how often to read the sensor (ms)
#ifndef USERMOD_VL53L0X_DISTANCE_MEASUREMENT_INTERVAL
#define USERMOD_VL53L0X_DISTANCE_MEASUREMENT_INTERVAL 1000
#endif

// max valid distance (mm); readings beyond this are treated as invalid
#ifndef USERMOD_VL53L0X_DISTANCE_MAX_MM
#define USERMOD_VL53L0X_DISTANCE_MAX_MM 2000
#endif

// timing budget for measurements (us)
#ifndef USERMOD_VL53L0X_DISTANCE_TIMING_BUDGET_US
#define USERMOD_VL53L0X_DISTANCE_TIMING_BUDGET_US 20000
#endif

class UsermodVL53L0XDistance : public Usermod {
  private:
    bool enabled = true;
    bool initDone = false;
    bool sensorFound = false;
    bool HApublished = false;

    unsigned long readingInterval = USERMOD_VL53L0X_DISTANCE_MEASUREMENT_INTERVAL;
    unsigned long lastMeasurement = UINT32_MAX - USERMOD_VL53L0X_DISTANCE_MEASUREMENT_INTERVAL;

    int16_t distanceMm = -1;

    VL53L0X sensor;

    static const char _name[];
    static const char _enabled[];
    static const char _readInterval[];
    static const char _sensor[];
    static const char _distance[];
    static const char _Distance[];

#ifndef WLED_DISABLE_MQTT
    void publishHomeAssistantAutodiscovery();
#endif

  public:
    void setup() override;
    void loop() override;
#ifndef WLED_DISABLE_MQTT
    void onMqttConnect(bool sessionPresent) override;
#endif
    void addToJsonInfo(JsonObject& root) override;
    void addToConfig(JsonObject& root) override;
    bool readFromConfig(JsonObject& root) override;
    uint16_t getId() override { return USERMOD_ID_VL53L0X_DISTANCE; }
};

#ifndef WLED_DISABLE_MQTT
void UsermodVL53L0XDistance::publishHomeAssistantAutodiscovery() {
  if (!WLED_MQTT_CONNECTED) return;

  char json_str[512], buf[128], uid[64];
  StaticJsonDocument<512> json;

  sprintf_P(buf, PSTR("%s Distance"), serverDescription);
  json[F("name")] = buf;

  strcpy(buf, mqttDeviceTopic);
  strcat_P(buf, _Distance);
  json[F("state_topic")] = buf;
  json[F("device_class")] = F("distance");
  snprintf(uid, sizeof(uid), "%s_distance", escapedMac.c_str());
  json[F("unique_id")] = uid;
  json[F("unit_of_measurement")] = F("mm");

  size_t payload_size = serializeJson(json, json_str);

  sprintf_P(buf, PSTR("homeassistant/sensor/%s/distance/config"), escapedMac.c_str());
  mqtt->publish(buf, 0, true, json_str, payload_size);
  HApublished = true;
}
#endif

void UsermodVL53L0XDistance::setup() {
  if (i2c_scl < 0 || i2c_sda < 0) {
    enabled = false;
    return;
  }

  sensor.setTimeout(150);
  sensorFound = sensor.init();
  if (!sensorFound) {
    DEBUG_PRINTLN(F("Failed to detect and initialize VL53L0X sensor!"));
  } else {
    sensor.setMeasurementTimingBudget(USERMOD_VL53L0X_DISTANCE_TIMING_BUDGET_US);
  }

  initDone = true;
}

void UsermodVL53L0XDistance::loop() {
  if (!enabled || !sensorFound || strip.isUpdating()) return;

  unsigned long now = millis();
  if (now - lastMeasurement < readingInterval) return;
  lastMeasurement = now;

  int16_t range = sensor.readRangeSingleMillimeters();
  if (sensor.timeoutOccurred() || range <= 0 || range > USERMOD_VL53L0X_DISTANCE_MAX_MM) {
    distanceMm = -1;
    return;
  }

  distanceMm = range;

#ifdef WLED_DEBUG
  DEBUG_PRINTF_P(PSTR("VL53L0X distance: %d mm\n"), distanceMm);
#endif

#ifndef WLED_DISABLE_MQTT
  if (WLED_MQTT_CONNECTED) {
    char subuf[128];
    strcpy(subuf, mqttDeviceTopic);
    strcat_P(subuf, _Distance);
    mqtt->publish(subuf, 0, false, String(distanceMm).c_str());
    strcat_P(subuf, PSTR("_cm"));
    mqtt->publish(subuf, 0, false, String(distanceMm / 10.0f, 1).c_str());
  }
#endif
}

#ifndef WLED_DISABLE_MQTT
void UsermodVL53L0XDistance::onMqttConnect(bool sessionPresent) {
  if (mqttDeviceTopic[0] != 0 && !HApublished) {
    publishHomeAssistantAutodiscovery();
  }
}
#endif

void UsermodVL53L0XDistance::addToJsonInfo(JsonObject& root) {
  if (!enabled) return;

  JsonObject user = root["u"];
  if (user.isNull()) user = root.createNestedObject("u");

  JsonArray dist = user.createNestedArray(FPSTR(_name));
  // if (distanceMm < 0) {
  //   dist.add(0);
  //   dist.add(F(" Sensor Error!"));
  //   return;
  // }
  dist.add(distanceMm);
  dist.add(F("mm"));

  JsonObject sensorObj = root[FPSTR(_sensor)];
  if (sensorObj.isNull()) sensorObj = root.createNestedObject(FPSTR(_sensor));
  dist = sensorObj.createNestedArray(FPSTR(_distance));
  dist.add(distanceMm);
  dist.add(F("mm"));
}

void UsermodVL53L0XDistance::addToConfig(JsonObject& root) {
  JsonObject top = root.createNestedObject(FPSTR(_name));
  top[FPSTR(_enabled)] = enabled;
  top[FPSTR(_readInterval)] = readingInterval;
}

bool UsermodVL53L0XDistance::readFromConfig(JsonObject& root) {
  DEBUG_PRINT(FPSTR(_name));

  JsonObject top = root[FPSTR(_name)];
  if (top.isNull()) {
    DEBUG_PRINTLN(F(": No config found. (Using defaults.)"));
    return false;
  }

  enabled = top[FPSTR(_enabled)] | enabled;
  readingInterval = top[FPSTR(_readInterval)] | readingInterval;
  readingInterval = min(60000UL, max(100UL, readingInterval));

  if (!initDone) {
    DEBUG_PRINTLN(F(" config loaded."));
  } else {
    DEBUG_PRINTLN(F(" config (re)loaded."));
  }
  return !top[FPSTR(_readInterval)].isNull();
}

const char UsermodVL53L0XDistance::_name[]         PROGMEM = "VL53L0XDistance";
const char UsermodVL53L0XDistance::_enabled[]      PROGMEM = "enabled";
const char UsermodVL53L0XDistance::_readInterval[] PROGMEM = "read-interval-ms";
const char UsermodVL53L0XDistance::_sensor[]       PROGMEM = "sensor";
const char UsermodVL53L0XDistance::_distance[]     PROGMEM = "distance";
const char UsermodVL53L0XDistance::_Distance[]     PROGMEM = "/distance";
