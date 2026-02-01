#include "web_pages.h"
#include "rules.h"
#include "io_catalog.h"
#include "app.h"


// minimal escaping
static String htmlEscape(String s) {
  s.replace("&", "&amp;");
  s.replace("\"", "&quot;");
  s.replace("<", "&lt;");
  s.replace(">", "&gt;");
  return s;
}

String buildMainHtml(const Settings& cfg, const String& ipStr, bool inApMode) {
  String p;
  p.reserve(1500);
  p += "<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>";
  p += "<title>Solar Heater</title><style>";
  p += "body{font-family:sans-serif;max-width:780px;margin:24px auto;padding:0 12px;}";
  p += ".card{border:1px solid #ddd;border-radius:14px;padding:14px;}";
  p += "img.logo{max-width:200px;width:100%;height:auto;display:block;margin:0 auto 10px;}";
  p += "a.btn{display:block;text-align:center;background:#eee;padding:12px;border-radius:12px;text-decoration:none;color:#000;font-size:18px;}";
  p += ".muted{color:#666;font-size:13px;}";
  p += "</style></head><body>";
  p += "<img src='/logo.png' class='logo' alt='Logo'>";
  p += "<h2>Solar Heater Controller</h2>";
  p += "<div class='card'>";
  p += "<p>Mode: <b>" + String(inApMode ? "AP setup" : "Normal") + "</b></p>";
  p += "<p>IP: <b>" + htmlEscape(ipStr) + "</b></p>";
  p += "<p>Tank setpoint: <b>" + String(cfg.tank_sp_c, 1) + " degC</b></p>";
  p += "<p class='muted'>Use config to edit settings and rules.</p>";
  p += "<a class='btn' href='/config'>Open Config</a>";
  p += "<div style='height:10px;'></div>";
  p += "<a class='btn' href='/rules'>Open Rules</a>";
  p += "</div></body></html>";
  return p;
}

// Small helper: page header + CSS
static void pageStart(String& p, const char* title) {
  p += "<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>";
  p += "<title>"; p += title; p += "</title><style>";
  p += "body{font-family:sans-serif;max-width:900px;margin:24px auto;padding:0 12px;}";
  p += "img.logo{max-width:220px;width:100%;height:auto;display:block;margin:0 auto 10px;}";
  p += ".card{border:1px solid #ddd;border-radius:14px;padding:14px;margin-top:12px;}";
  p += "label{display:block;font-weight:600;margin:10px 0 6px;}";
  p += "input,select{width:100%;font-size:16px;padding:10px;border-radius:10px;border:1px solid #ccc;box-sizing:border-box;}";
  p += ".row{display:grid;grid-template-columns:1fr 1fr;gap:12px;}";
  p += "button{font-size:18px;padding:10px 16px;border-radius:12px;border:0;cursor:pointer;width:100%;}";
  p += "a.btn{display:block;text-align:center;background:#eee;padding:12px;border-radius:12px;text-decoration:none;color:#000;font-size:18px;margin-top:10px;}";
  p += "a.btn2{display:inline-block;text-align:center;background:#eee;padding:10px 14px;border-radius:12px;text-decoration:none;color:#000;font-size:16px;margin:6px 6px 0 0;}";
  p += ".muted{color:#666;font-size:13px;line-height:1.35;}";
  p += "</style></head><body>";
  p += "<img src='/logo.png' class='logo' alt='Logo'>";
}

static void pageEnd(String& p) {
  p += "</body></html>";
}

static String saveButtons(const String& returnPath) {
  String s;
  s += "<div class='card'>";
  s += "<input type='hidden' name='return' value='" + htmlEscape(returnPath) + "'>";
  s += "<button type='submit'>Save</button>";
  s += "<a class='btn' href='/config'>Back to Config Home</a>";
  s += "</div>";
  return s;
}

String buildConfigHomeHtml(const Settings& cfg, bool inApMode) {
  (void)cfg;
  String p; p.reserve(4000);
  pageStart(p, "Config");

  p += "<h2>Configuration</h2>";
  p += "<div class='card'>";
  p += "<div class='muted'>Pick a section. Settings are stored in NVS and survive reboot.</div>";
  p += "<div style='margin-top:8px;'>";
  p += "<a class='btn2' href='/config/control'>Control</a>";
  p += "<a class='btn2' href='/config/shunts'>Shunts</a>";
  p += "<a class='btn2' href='/config/relays'>Relays</a>";
  p += "<a class='btn2' href='/config/pv'>PV Model</a>";
  p += "<a class='btn2' href='/config/elements'>Elements</a>";
  p += "<a class='btn2' href='/config/mqtt'>MQTT</a>";
  p += "<a class='btn2' href='/config/wifi'>Wi-Fi</a>";
  p += "<a class='btn2' href='/rules'>Rules</a>";
  p += "<a class='btn2' href='/'>Home</a>";
  p += "</div>";

  if (inApMode) {
    p += "<p class='muted' style='margin-top:10px;'><b>AP setup mode:</b> Wi-Fi section is where you enter your network credentials.</p>";
  }
  p += "</div>";

  pageEnd(p);
  return p;
}

String buildConfigControlHtml(const Settings& cfg) {
  String p; p.reserve(5000);
  pageStart(p, "Control");

  p += "<h2>Control</h2>";
  p += "<form method='POST' action='/saveSettings'>";
  p += "<div class='card'>";
  p += "<label>Tank setpoint (degC)</label>";
  p += "<input name='tank_sp_c' type='number' step='0.1' value='" + String(cfg.tank_sp_c, 1) + "'>";

  p += "<div class='row'>";
  p += "<div><label>Timezone offset (minutes)</label>";
  p += "<input name='tz_offset_min' type='number' step='1' value='" + String(cfg.tz_offset_min) + "'></div>";
  p += "<div><label>RTC epoch (seconds)</label>";
  p += "<input name='rtc_epoch' type='number' step='1' value='" + String((long long)cfg.rtc_epoch) + "'></div>";
  p += "</div>";

  p += "<label>Heartbeat blink (ms)</label>";
  p += "<input name='ms' type='number' min='10' max='60000' value='" + String(app.blinkMs) + "'>";

  p += "<div class='muted' style='margin-top:8px;'>RTC epoch is optional; later you can add NTP and ignore this.</div>";
  p += "</div>";

  p += saveButtons("/config/control");
  p += "</form>";

  pageEnd(p);
  return p;
}

String buildConfigMqttHtml(const Settings& cfg) {
  String p; p.reserve(5000);
  pageStart(p, "MQTT");

  p += "<h2>MQTT</h2>";
  p += "<form method='POST' action='/saveSettings'>";
  p += "<div class='card'>";
  p += "<div class='row'>";
  p += "<div><label>Host</label><input name='mqtt_host' value='" + htmlEscape(cfg.mqtt_host) + "'></div>";
  p += "<div><label>Port</label><input name='mqtt_port' type='number' value='" + String(cfg.mqtt_port) + "'></div>";
  p += "</div>";

  p += "<div class='row'>";
  p += "<div><label>User</label><input name='mqtt_user' value='" + htmlEscape(cfg.mqtt_user) + "'></div>";
  p += "<div><label>Password</label><input name='mqtt_pass' type='password' value='" + htmlEscape(cfg.mqtt_pass) + "'></div>";
  p += "</div>";

  p += "<label>Base topic</label><input name='mqtt_base' value='" + htmlEscape(cfg.mqtt_base) + "'>";
  p += "</div>";

  p += saveButtons("/config/mqtt");
  p += "</form>";

  pageEnd(p);
  return p;
}

String buildConfigWifiHtml(const Settings& cfg, bool inApMode) {
  String p; p.reserve(5500);
  pageStart(p, "Wi-Fi");

  p += "<h2>Wi-Fi</h2>";
  p += "<form method='POST' action='/saveSettings'>";

  p += "<div class='card'>";
  p += "<label>Wi-Fi SSID</label>";
  p += "<input name='wifi_ssid' value='" + htmlEscape(cfg.wifi_ssid) + "'" + String(inApMode ? " required" : "") + ">";

  p += "<label>Wi-Fi Password</label>";
  p += "<input name='wifi_pass' type='password' value='" + htmlEscape(cfg.wifi_pass) + "'" + String(inApMode ? " required" : "") + ">";

  p += "<label>Device AP password (optional, 8+ chars)</label>";
  p += "<input name='ap_pass' value='" + htmlEscape(cfg.ap_pass) + "'>";

  if (inApMode) {
    p += "<div class='muted' style='margin-top:8px;'>Saving in AP mode will reboot and attempt to join the Wi-Fi network.</div>";
  } else {
    p += "<div class='muted' style='margin-top:8px;'>If you change creds while already connected, you may need to reboot to apply.</div>";
  }
  p += "</div>";

  p += saveButtons("/config/wifi");
  p += "</form>";

  p += "<div class='card'>";
  p += "<form method='POST' action='/forgetWiFi'>";
  p += "<button type='submit'>Forget Wi-Fi and reboot into AP mode</button>";
  p += "</form>";
  p += "</div>";

  pageEnd(p);
  return p;
}

String buildConfigShuntsHtml(const Settings& cfg) {
  String p; p.reserve(8000);
  pageStart(p, "Shunts");

  p += "<h2>Shunts</h2>";
  p += "<form method='POST' action='/saveSettings'>";

  p += "<div class='card'>";
  p += "<label>Shunt mode</label>";
  p += "<select name='shunt_mode'>";
  p += "<option value='rated' " + String(cfg.shunt_mode == "rated" ? "selected" : "") + ">rated (A/mV)</option>";
  p += "<option value='mohm' "  + String(cfg.shunt_mode == "mohm"  ? "selected" : "") + ">mOhm override</option>";
  p += "</select>";
  p += "<div class='muted' style='margin-top:8px;'>If mode=mOhm, firmware uses the mOhm override; otherwise it computes mOhm from rated A/mV.</div>";
  p += "</div>";

  p += "<div class='card'><h3 style='margin:0 0 8px;'>PV shunt</h3>";
  p += "<div class='row'>";
  p += "<div><label>Rated amps</label><input name='pv_shunt_a' type='number' value='" + String(cfg.pv_shunt_a) + "'></div>";
  p += "<div><label>Rated mV</label><input name='pv_shunt_mv' type='number' value='" + String(cfg.pv_shunt_mv) + "'></div>";
  p += "</div>";
  p += "<label>mOhm override</label><input name='pv_shunt_mohm' type='number' step='0.0001' value='" + String(cfg.pv_shunt_mohm, 4) + "'>";
  p += "</div>";

  p += "<div class='card'><h3 style='margin:0 0 8px;'>Main shunt</h3>";
  p += "<div class='row'>";
  p += "<div><label>Rated amps</label><input name='main_shunt_a' type='number' value='" + String(cfg.main_shunt_a) + "'></div>";
  p += "<div><label>Rated mV</label><input name='main_shunt_mv' type='number' value='" + String(cfg.main_shunt_mv) + "'></div>";
  p += "</div>";
  p += "<label>mOhm override</label><input name='main_shunt_mohm' type='number' step='0.0001' value='" + String(cfg.main_shunt_mohm, 4) + "'>";
  p += "</div>";

  p += saveButtons("/config/shunts");
  p += "</form>";

  pageEnd(p);
  return p;
}

String buildConfigRelaysHtml(const Settings& cfg) {
  String p; p.reserve(7000);
  pageStart(p, "Relays");

  p += "<h2>Relay assignments</h2>";
  p += "<form method='POST' action='/saveSettings'>";

  auto locSelect = [&](const char* name, const String& cur) {
    String s;
    s += "<select name='"; s += name; s += "'>";
    s += "<option value='master' " + String(cur == "master" ? "selected" : "") + ">master</option>";
    s += "<option value='remote' " + String(cur == "remote" ? "selected" : "") + ">remote</option>";
    s += "</select>";
    return s;
  };

  p += "<div class='card'>";
  p += "<h3 style='margin:0 0 8px;'>AC diversion relay</h3>";
  p += "<div class='row'>";
  p += "<div><label>Location</label>" + locSelect("div_loc", cfg.div_loc) + "</div>";
  p += "<div><label>Relay index</label><input name='div_idx' type='number' min='1' max='3' value='" + String(cfg.div_idx) + "'></div>";
  p += "</div>";
  p += "<div class='muted' style='margin-top:8px;'>Master supports relay 1-2; Remote supports relay 1-3. Firmware clamps if needed.</div>";
  p += "</div>";

  p += "<div class='card'>";
  p += "<h3 style='margin:0 0 8px;'>PV safety contactor (kill PV)</h3>";
  p += "<div class='row'>";
  p += "<div><label>Location</label>" + locSelect("pvkill_loc", cfg.pvkill_loc) + "</div>";
  p += "<div><label>Relay index</label><input name='pvkill_idx' type='number' min='1' max='3' value='" + String(cfg.pvkill_idx) + "'></div>";
  p += "</div>";
  p += "</div>";

  p += saveButtons("/config/relays");
  p += "</form>";

  pageEnd(p);
  return p;
}

String buildConfigPvHtml(const Settings& cfg) {
  String p; p.reserve(6500);
  pageStart(p, "PV Model");

  p += "<h2>PV model</h2>";
  p += "<form method='POST' action='/saveSettings'>";

  p += "<div class='card'>";
  p += "<div class='row'>";
  p += "<div><label>Panels in series (Ns)</label><input name='pv_ns' type='number' min='1' value='" + String(cfg.pv_ns) + "'></div>";
  p += "<div><label>Panels in parallel (Np)</label><input name='pv_np' type='number' min='1' value='" + String(cfg.pv_np) + "'></div>";
  p += "</div>";

  p += "<div class='row'>";
  p += "<div><label>Vmp per panel</label><input name='pv_vmp' type='number' step='0.1' value='" + String(cfg.pv_vmp, 1) + "'></div>";
  p += "<div><label>Voc per panel</label><input name='pv_voc' type='number' step='0.1' value='" + String(cfg.pv_voc, 1) + "'></div>";
  p += "</div>";

  p += "<label>Imp per panel</label><input name='pv_imp' type='number' step='0.1' value='" + String(cfg.pv_imp, 1) + "'>";
  p += "<div class='muted' style='margin-top:8px;'>Used for estimates and sanity checks. Doesn’t need to be perfect.</div>";
  p += "</div>";

  p += saveButtons("/config/pv");
  p += "</form>";

  pageEnd(p);
  return p;
}

String buildConfigElementsHtml(const Settings& cfg) {
  String p; p.reserve(12000);
  pageStart(p, "Elements");

  p += "<h2>Heating elements</h2>";
  p += "<form method='POST' action='/saveSettings'>";

  p += "<div class='card'>";
  p += "<div class='muted'>Enter nominal voltage and wattage for up to 8 elements.</div>";

  for (int i = 0; i < 8; i++) {
    p += "<div class='row'>";
    p += "<div><label>Element " + String(i+1) + " voltage</label>";
    p += "<input name='elv" + String(i) + "' type='number' step='0.1' value='" + String(cfg.el_v[i], 1) + "'></div>";
    p += "<div><label>Element " + String(i+1) + " wattage</label>";
    p += "<input name='elw" + String(i) + "' type='number' step='1' value='" + String(cfg.el_w[i], 0) + "'></div>";
    p += "</div>";
  }

  p += "<label><input type='checkbox' name='learn_elems' " + String(cfg.learn_elems ? "checked" : "") + "> Learn heating elements (experimental)</label>";
  p += "</div>";

  p += saveButtons("/config/elements");
  p += "</form>";

  pageEnd(p);
  return p;
}


// FULL rules page (RHS const/input) — you asked to keep this unified
String buildRulesHtml() {
  String p;
  p.reserve(16000);

  p += "<!doctype html><html><head>";
  p += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  p += "<title>Rules</title>";
  p += "<style>";
  p += "body{font-family:sans-serif;max-width:1200px;margin:20px auto;padding:0 12px;}";
  p += ".row{display:flex;gap:12px;flex-wrap:wrap;align-items:center;}";
  p += ".muted{color:#666;font-size:13px;line-height:1.35;}";
  p += "table{width:100%;border-collapse:collapse;margin-top:12px;table-layout:fixed;}";
  p += "th,td{border:1px solid #ddd;padding:8px;font-size:14px;vertical-align:top;overflow:hidden;}";
  p += "th{background:#f6f6f6;text-align:left;}";
  p += "select,input{font-size:14px;padding:6px;width:100%;box-sizing:border-box;}";
  p += ".tight{width:1%;white-space:nowrap;}";
  // Column width classes
  p += ".col-en{width:46px;}";
  p += ".col-in{width:190px;}";
  p += ".col-op{width:80px;}";
  p += ".col-rhs{width:86px;}";
  p += ".col-rhsval{width:340px;}";   // this cell contains BOTH controls
  p += ".col-out{width:170px;}";
  p += ".col-act{width:88px;}";
  p += ".col-mode{width:96px;}";
  p += ".col-dur{width:90px;}";

  // RHS two-control layout: make them fixed-width, not 50/50
  p += ".rhsgrid{display:grid;grid-template-columns:120px 180px;gap:8px;}";

  p += "button{font-size:16px;padding:10px 14px;border-radius:10px;border:0;cursor:pointer;}";
  p += "a.btn{display:inline-block;text-decoration:none;padding:10px 14px;background:#eee;border-radius:10px;color:#000;}";
  p += ".tight{width:1%;white-space:nowrap;}";
  p += "</style></head><body>";

  p += "<div class='row'>";
  p += "<img src='/logo.png' style='max-width:140px;height:auto;'>";
  p += "<div>";
  p += "<h2 style='margin:0;'>Programmable Outputs</h2>";
  p += "<div class='muted'>Rules: IF input op RHS THEN output action. Modes: follow / once / timed. Duration in seconds.</div>";
  p += "</div></div>";

  p += "<form method='POST' action='/saveRules'>";
  p += "<table>";
  p += "<tr>";
p += "<th class='col-en'>En</th>";
p += "<th class='col-in'>Input</th>";
p += "<th class='col-op'>Op</th>";
p += "<th class='col-rhs'>RHS</th>";
p += "<th class='col-rhsval'>Value (const) / Input (rhs)</th>";
p += "<th class='col-out'>Output</th>";
p += "<th class='col-act'>Action</th>";
p += "<th class='col-mode'>Mode</th>";
p += "<th class='col-dur'>Dur (sec)</th>";
  p += "</tr>";

  for (int i = 0; i < MAX_RULES; i++) {
    String idx = String(i);
    String base = "r" + idx + "_";

    p += "<tr>";

    // Enabled
    p += "<td class='tight' style='text-align:center;'>";
    p += "<input type='checkbox' name='" + base + "en' " + String(rules[i].enabled ? "checked" : "") + ">";
    p += "</td>";

    // LHS input
    p += "<td><select name='" + base + "in'>";
    for (int k = 0; k < N_INPUTS; k++) {
      String key = INPUT_KEYS[k];
      p += "<option value='" + key + "' " + String(rules[i].inputKey == key ? "selected" : "") + ">" + key + "</option>";
    }
    p += "</select></td>";

    // Operator
    String opStr = String(opToStr(rules[i].op));
    p += "<td><select name='" + base + "op'>";
    const char* ops[] = { ">", ">=", "<", "<=", "==", "!=" };
    for (const char* s : ops) {
      p += "<option value='" + String(s) + "' " + String(opStr == s ? "selected" : "") + ">" + String(s) + "</option>";
    }
    p += "</select></td>";

    // RHS source
    p += "<td><select name='" + base + "rhs'>";
    p += "<option value='const' " + String(rules[i].rhsSource == RhsSource::CONST_VAL ? "selected" : "") + ">const</option>";
    p += "<option value='input' " + String(rules[i].rhsSource == RhsSource::INPUT_KEY ? "selected" : "") + ">input</option>";
    p += "</select></td>";

    // RHS value (both shown)
    p += "<td style='min-width:240px;'>";
    p += "<div class='rhsgrid'>";
    p += "<input name='" + base + "th' type='number' step='0.01' value='" + String(rules[i].threshold, 2) + "' title='constant value'>";
    p += "<select name='" + base + "rin' title='rhs input'>";
    for (int k = 0; k < N_INPUTS; k++) {
      String key = INPUT_KEYS[k];
      p += "<option value='" + key + "' " + String(rules[i].rhsInputKey == key ? "selected" : "") + ">" + key + "</option>";
    }
    p += "</select>";
    p += "</div></td>";

    // Output
    p += "<td><select name='" + base + "out'>";
    for (int k = 0; k < N_OUTPUTS; k++) {
      String key = OUTPUT_KEYS[k];
      p += "<option value='" + key + "' " + String(rules[i].outputKey == key ? "selected" : "") + ">" + key + "</option>";
    }
    p += "</select></td>";

    // Action
    p += "<td><select name='" + base + "on'>";
    p += "<option value='1' " + String(rules[i].outputOn ? "selected" : "") + ">ON</option>";
    p += "<option value='0' " + String(!rules[i].outputOn ? "selected" : "") + ">OFF</option>";
    p += "</select></td>";

    // Mode
    p += "<td><select name='" + base + "mode'>";
    p += "<option value='follow' " + String(rules[i].mode == RuleMode::FOLLOW ? "selected" : "") + ">follow</option>";
    p += "<option value='once' "   + String(rules[i].mode == RuleMode::ONCE   ? "selected" : "") + ">once</option>";
    p += "<option value='timed' "  + String(rules[i].mode == RuleMode::TIMED  ? "selected" : "") + ">timed</option>";
    p += "</select></td>";

    // Duration
    p += "<td><input name='" + base + "dur' type='number' min='0' max='86400' value='" + String(rules[i].durationSec) + "'></td>";

    p += "</tr>";
  }

  p += "</table>";

  p += "<div class='row' style='margin-top:12px;'>";
  p += "<button type='submit'>Save Rules</button>";
  p += "<a class='btn' href='/'>Back</a>";
  p += "<a class='btn' href='/config'>Config</a>";
  p += "</div>";

  p += "<div class='muted' style='margin-top:10px;'>";
  p += "Tip: RHS=const uses the numeric value. RHS=input compares against the selected RHS input.";
  p += "</div>";

  p += "</form></body></html>";

  return p;
}
