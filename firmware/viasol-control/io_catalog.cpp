#include "io_catalog.h"
#include "app.h"

#ifndef LED_BUILTIN
  #define LED_BUILTIN 2
#endif
static const int LED_PIN = LED_BUILTIN;

const char* INPUT_KEYS[] = {
  "tank_temp_c", 
  "floor_temp_c", 
  "indoor_temp_c",
  "tank_setpoint_c",
  "indoor_setpoint_c",
  "soc",
  "pv_v", 
  "pv_a", 
  "main_a",
  "mqtt1", "mqtt2", "mqtt3", "mqtt4"
};
const int N_INPUTS = sizeof(INPUT_KEYS) / sizeof(INPUT_KEYS[0]);

const char* OUTPUT_KEYS[] = {
  "m_relay1","m_relay2",
  "r_relay1","r_relay2","r_relay3",
  "m_aux1","m_aux2","m_aux3",
  "r_aux1","r_aux2","r_aux3"
};
const int N_OUTPUTS = sizeof(OUTPUT_KEYS) / sizeof(OUTPUT_KEYS[0]);

float inputValueByKey(const String& key) {
  // TODO: wire these to real sensors + MQTT "virtual registers"
  if (key == "tank_temp_c") return 42.0f;
  if (key == "pv_v") return 80.0f;
  if (key == "pv_a") return 10.0f;
  if (key == "main_a") return 2.0f;

  // mqtt placeholders
  if (key.startsWith("mqtt")) return 0.0f;

  return 0.0f;
}

void applyOutput(const String& outputKey, bool on) {
  // TODO: map to actual relays/aux lines
  // For sanity, drive the onboard LED via one output
  if (outputKey == "m_aux1") {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, on ? HIGH : LOW);
  }

  // Debug print (remove later or gate with a debug flag)
  // Serial.printf("OUTPUT %s = %s\n", outputKey.c_str(), on ? "ON" : "OFF");
}
