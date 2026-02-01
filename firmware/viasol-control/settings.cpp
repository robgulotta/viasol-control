#include "settings.h"
#include "app.h"
#include <Preferences.h>

static void clampRelayIdx(const String& loc, int& idx) {
  int maxIdx = (loc == "master") ? 2 : 3;
  if (idx < 1) idx = 1;
  if (idx > maxIdx) idx = maxIdx;
}

void validateSettings(Settings& cfg) {
  clampRelayIdx(cfg.div_loc, cfg.div_idx);
  clampRelayIdx(cfg.pvkill_loc, cfg.pvkill_idx);

  if (cfg.mqtt_port < 1) cfg.mqtt_port = 1883;
  if (cfg.mqtt_port > 65535) cfg.mqtt_port = 65535;

  if (cfg.pv_ns < 1) cfg.pv_ns = 1;
  if (cfg.pv_np < 1) cfg.pv_np = 1;
}

void loadSettings(Settings& cfg) {
  app.prefs.begin("settings", false);

  cfg.tank_sp_c = app.prefs.getFloat("tank_sp_c", cfg.tank_sp_c);

  cfg.shunt_mode = app.prefs.getString("shunt_mode", cfg.shunt_mode);
  cfg.pv_shunt_a = app.prefs.getInt("pv_shunt_a", cfg.pv_shunt_a);
  cfg.pv_shunt_mv = app.prefs.getInt("pv_shunt_mv", cfg.pv_shunt_mv);
  cfg.pv_shunt_mohm = app.prefs.getFloat("pv_shunt_mohm", cfg.pv_shunt_mohm);

  cfg.main_shunt_a = app.prefs.getInt("main_shunt_a", cfg.main_shunt_a);
  cfg.main_shunt_mv = app.prefs.getInt("main_shunt_mv", cfg.main_shunt_mv);
  cfg.main_shunt_mohm = app.prefs.getFloat("main_shunt_mohm", cfg.main_shunt_mohm);

  cfg.tz_offset_min = app.prefs.getInt("tz_offset_min", cfg.tz_offset_min);
  cfg.rtc_epoch = (int64_t)app.prefs.getLong64("rtc_epoch", cfg.rtc_epoch);

  cfg.div_loc = app.prefs.getString("div_loc", cfg.div_loc);
  cfg.div_idx = app.prefs.getInt("div_idx", cfg.div_idx);
  cfg.pvkill_loc = app.prefs.getString("pvkill_loc", cfg.pvkill_loc);
  cfg.pvkill_idx = app.prefs.getInt("pvkill_idx", cfg.pvkill_idx);

  cfg.m_aux1_name = app.prefs.getString("m_aux1_name", cfg.m_aux1_name);
  cfg.m_aux2_name = app.prefs.getString("m_aux2_name", cfg.m_aux2_name);
  cfg.m_aux3_name = app.prefs.getString("m_aux3_name", cfg.m_aux3_name);
  cfg.r_aux1_name = app.prefs.getString("r_aux1_name", cfg.r_aux1_name);
  cfg.r_aux2_name = app.prefs.getString("r_aux2_name", cfg.r_aux2_name);
  cfg.r_aux3_name = app.prefs.getString("r_aux3_name", cfg.r_aux3_name);

  cfg.mqtt_host = app.prefs.getString("mqtt_host", cfg.mqtt_host);
  cfg.mqtt_port = app.prefs.getInt("mqtt_port", cfg.mqtt_port);
  cfg.mqtt_user = app.prefs.getString("mqtt_user", cfg.mqtt_user);
  cfg.mqtt_pass = app.prefs.getString("mqtt_pass", cfg.mqtt_pass);
  cfg.mqtt_base = app.prefs.getString("mqtt_base", cfg.mqtt_base);

  cfg.pv_ns = app.prefs.getInt("pv_ns", cfg.pv_ns);
  cfg.pv_np = app.prefs.getInt("pv_np", cfg.pv_np);
  cfg.pv_vmp = app.prefs.getFloat("pv_vmp", cfg.pv_vmp);
  cfg.pv_voc = app.prefs.getFloat("pv_voc", cfg.pv_voc);
  cfg.pv_imp = app.prefs.getFloat("pv_imp", cfg.pv_imp);

  for (int i = 0; i < 8; i++) {
    char key[8];
    snprintf(key, sizeof(key), "elv%d", i);
    cfg.el_v[i] = app.prefs.getFloat(key, cfg.el_v[i]);
    snprintf(key, sizeof(key), "elw%d", i);
    cfg.el_w[i] = app.prefs.getFloat(key, cfg.el_w[i]);
  }
  cfg.learn_elems = app.prefs.getBool("learn_elems", cfg.learn_elems);

  cfg.wifi_ssid = app.prefs.getString("wifi_ssid", cfg.wifi_ssid);
  cfg.wifi_pass = app.prefs.getString("wifi_pass", cfg.wifi_pass);
  cfg.ap_pass   = app.prefs.getString("ap_pass", cfg.ap_pass);

  // optional
  app.blinkMs = app.prefs.getUInt("blinkMs", app.blinkMs);

  validateSettings(cfg);
}

void saveSettings(const Settings& cfg) {
  app.prefs.putFloat("tank_sp_c", cfg.tank_sp_c);

  app.prefs.putString("shunt_mode", cfg.shunt_mode);
  app.prefs.putInt("pv_shunt_a", cfg.pv_shunt_a);
  app.prefs.putInt("pv_shunt_mv", cfg.pv_shunt_mv);
  app.prefs.putFloat("pv_shunt_mohm", cfg.pv_shunt_mohm);

  app.prefs.putInt("main_shunt_a", cfg.main_shunt_a);
  app.prefs.putInt("main_shunt_mv", cfg.main_shunt_mv);
  app.prefs.putFloat("main_shunt_mohm", cfg.main_shunt_mohm);

  app.prefs.putInt("tz_offset_min", cfg.tz_offset_min);
  app.prefs.putLong64("rtc_epoch", (int64_t)cfg.rtc_epoch);

  app.prefs.putString("div_loc", cfg.div_loc);
  app.prefs.putInt("div_idx", cfg.div_idx);
  app.prefs.putString("pvkill_loc", cfg.pvkill_loc);
  app.prefs.putInt("pvkill_idx", cfg.pvkill_idx);

  app.prefs.putString("m_aux1_name", cfg.m_aux1_name);
  app.prefs.putString("m_aux2_name", cfg.m_aux2_name);
  app.prefs.putString("m_aux3_name", cfg.m_aux3_name);
  app.prefs.putString("r_aux1_name", cfg.r_aux1_name);
  app.prefs.putString("r_aux2_name", cfg.r_aux2_name);
  app.prefs.putString("r_aux3_name", cfg.r_aux3_name);

  app.prefs.putString("mqtt_host", cfg.mqtt_host);
  app.prefs.putInt("mqtt_port", cfg.mqtt_port);
  app.prefs.putString("mqtt_user", cfg.mqtt_user);
  app.prefs.putString("mqtt_pass", cfg.mqtt_pass);
  app.prefs.putString("mqtt_base", cfg.mqtt_base);

  app.prefs.putInt("pv_ns", cfg.pv_ns);
  app.prefs.putInt("pv_np", cfg.pv_np);
  app.prefs.putFloat("pv_vmp", cfg.pv_vmp);
  app.prefs.putFloat("pv_voc", cfg.pv_voc);
  app.prefs.putFloat("pv_imp", cfg.pv_imp);

  for (int i = 0; i < 8; i++) {
    char key[8];
    snprintf(key, sizeof(key), "elv%d", i);
    app.prefs.putFloat(key, cfg.el_v[i]);
    snprintf(key, sizeof(key), "elw%d", i);
    app.prefs.putFloat(key, cfg.el_w[i]);
  }
  app.prefs.putBool("learn_elems", cfg.learn_elems);

  app.prefs.putString("wifi_ssid", cfg.wifi_ssid);
  app.prefs.putString("wifi_pass", cfg.wifi_pass);
  app.prefs.putString("ap_pass", cfg.ap_pass);

  app.prefs.putUInt("blinkMs", app.blinkMs);
}
