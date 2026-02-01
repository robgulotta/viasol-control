#pragma once
#include <Arduino.h>

struct Settings {
  // Control
  float tank_sp_c = 55.0f;

  // Shunts
  String shunt_mode = "rated"; // "rated" or "mohm"
  int pv_shunt_a = 300;
  int pv_shunt_mv = 75;
  float pv_shunt_mohm = 0.25f;

  int main_shunt_a = 500;
  int main_shunt_mv = 50;
  float main_shunt_mohm = 0.10f;

  // Time
  int tz_offset_min = -420;
  int64_t rtc_epoch = 0;

  // Relays
  String div_loc = "master"; // master|remote
  int div_idx = 1;
  String pvkill_loc = "master";
  int pvkill_idx = 2;

  // Names
  String m_aux1_name = "master_aux1";
  String m_aux2_name = "master_aux2";
  String m_aux3_name = "master_aux3";
  String r_aux1_name = "remote_aux1";
  String r_aux2_name = "remote_aux2";
  String r_aux3_name = "remote_aux3";

  // MQTT
  String mqtt_host = "";
  int mqtt_port = 1883;
  String mqtt_user = "";
  String mqtt_pass = "";
  String mqtt_base = "solarheater";

  // PV model
  int pv_ns = 4;
  int pv_np = 2;
  float pv_vmp = 34.0f;
  float pv_voc = 41.0f;
  float pv_imp = 8.5f;

  // Heating elements
  float el_v[8] = {24,24,24,24,24,24,24,24};
  float el_w[8] = {1000,1000,1000,1000,1000,1000,1000,1000};
  bool learn_elems = false;

  // Wi-Fi creds
  String wifi_ssid = "";
  String wifi_pass = "";

  // AP password (optional)
  String ap_pass = "";
};

void loadSettings(Settings& cfg);
void saveSettings(const Settings& cfg);
void validateSettings(Settings& cfg);
