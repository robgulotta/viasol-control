#include "web_routes.h"
#include "app.h"
#include "web_pages.h"
#include "settings.h"
#include "rules.h"
#include "rules2.h"
#include "web_pages_rules2.h"
#include "web_pages_rules2_groups.h"


#include <WiFi.h>
#include <LittleFS.h>

static void handleLogo() {

  uint32_t t0 = millis();
  Serial.println("[http] logo start");

  File f = LittleFS.open("/logo.png", "r");
  if (!f) {
    app.server.send(404, "text/plain", "Missing /logo.png in LittleFS");
    return;
  }
  app.server.sendHeader("Cache-Control", "public, max-age=86400");
  app.server.sendHeader("Connection", "close");
  app.server.streamFile(f, "image/png");
  f.close();

  Serial.printf("[http] logo done in %u ms\n", (unsigned)(millis() - t0));

}

static int argInt(const char* name, int cur) {
  if (!app.server.hasArg(name)) return cur;
  return app.server.arg(name).toInt();
}
static float argFloat(const char* name, float cur) {
  if (!app.server.hasArg(name)) return cur;
  return app.server.arg(name).toFloat();
}
static String argStr(const char* name, const String& cur) {
  if (!app.server.hasArg(name)) return cur;
  return app.server.arg(name);
}
static bool argBool(const char* name) {
  return app.server.hasArg(name);
}

static void handleMain(Settings& cfg) {
  String ipStr = app.inApMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
  app.server.send(200, "text/html", buildMainHtml(cfg, ipStr, app.inApMode));
}

static void handleRules() {
  app.server.send(200, "text/html", buildRulesHtml());
}

static void handleSaveRules() {
  for (int i = 0; i < MAX_RULES; i++) {
    String base = "r" + String(i) + "_";

    rules[i].enabled = app.server.hasArg(base + "en");

    if (app.server.hasArg(base + "in"))   rules[i].inputKey = app.server.arg(base + "in");
    if (app.server.hasArg(base + "op"))   rules[i].op = strToOp(app.server.arg(base + "op"));

    if (app.server.hasArg(base + "rhs"))  rules[i].rhsSource = strToRhs(app.server.arg(base + "rhs"));
    if (app.server.hasArg(base + "th"))   rules[i].threshold = app.server.arg(base + "th").toFloat();
    if (app.server.hasArg(base + "rin"))  rules[i].rhsInputKey = app.server.arg(base + "rin");

    if (app.server.hasArg(base + "out"))  rules[i].outputKey = app.server.arg(base + "out");
    if (app.server.hasArg(base + "on"))   rules[i].outputOn = (app.server.arg(base + "on").toInt() != 0);
    if (app.server.hasArg(base + "mode")) rules[i].mode = strToMode(app.server.arg(base + "mode"));
    if (app.server.hasArg(base + "dur"))  rules[i].durationSec = (uint32_t)app.server.arg(base + "dur").toInt();
  }

  saveRules();
  app.server.sendHeader("Location", "/rules");
  app.server.send(303);
}

// ---- Rules v2 handlers (parallel, does not touch rules.h) ----

static void handleRules2(Settings& cfg) {
  app.server.send(200, "text/html", buildRules2HomeHtml(cfg));
}

static void handleRules2Conditions(Settings& cfg) {
  app.server.send(200, "text/html", buildRules2ConditionsHtml(cfg));
}

static void handleRules2EditRule(Settings& cfg) {
  uint32_t id = (uint32_t)app.server.arg("id").toInt();
  app.server.send(200, "text/html", buildRules2EditRuleHtml(cfg, id));
}

static void handleRules2NewRule() {
  // Create a blank rule with a blank expression root (we'll make a default leaf)
  uint32_t rid = rules2::uiCreateDefaultRule();
  app.server.sendHeader("Location", String("/config/rules2/edit?id=") + rid);
  app.server.send(303);
}

static void handleRules2DeleteRule() {
  uint32_t id = (uint32_t)app.server.arg("id").toInt();
  rules2::uiDeleteRule(id);
  rules2::saveRules2(); // optional now, but good habit
  app.server.sendHeader("Location", "/config/rules2");
  app.server.send(303);
}

static void handleRules2SaveRule() {
  Serial.println("[rules2] save: begin");
  rules2::uiSaveRuleFromPost(app.server);
  Serial.println("[rules2] save: after uiSaveRuleFromPost");

  rules2::saveRules2();
  Serial.println("[rules2] save: after saveRules2");

  app.server.sendHeader("Connection", "close");
  app.server.sendHeader("Location", "/config/rules2");
  app.server.send(303, "text/plain", "");
  Serial.println("[rules2] save: response sent");
}


static void handleRules2NewCondition() {
  uint32_t cid = rules2::uiCreateDefaultCondition();
  app.server.sendHeader("Location", String("/config/rules2/conditions"));
  app.server.send(303);
}

static void handleRules2DeleteCondition() {
  uint32_t id = (uint32_t)app.server.arg("id").toInt();
  rules2::uiDeleteCondition(id);
  rules2::saveRules2();
  app.server.sendHeader("Location", "/config/rules2/conditions");
  app.server.send(303);
}

static void handleRules2SaveConditions() {
  rules2::uiSaveConditionsFromPost(app.server);
  rules2::saveRules2();
  app.server.sendHeader("Location", "/config/rules2/conditions");
  app.server.send(303);
}

// quick bootstrap/demo (optional)
static void handleRules2Demo() {
  rules2::initRules2Defaults();
  rules2::saveRules2();
  app.server.sendHeader("Location", "/config/rules2");
  app.server.send(303);
}

static void handleRules2Groups(Settings& cfg) {
  app.server.send(200, "text/html", buildRules2GroupsHtml(cfg));
}

static void handleRules2EditGroup(Settings& cfg) {
  uint32_t id = (uint32_t)app.server.arg("id").toInt();
  app.server.send(200, "text/html", buildRules2EditGroupHtml(cfg, id));
}

static void handleRules2NewGroup() {
  String name = app.server.hasArg("name") ? app.server.arg("name") : "New group";
  String t = app.server.hasArg("gtype") ? app.server.arg("gtype") : "AND";
  bool isOr = (t == "OR");
  uint32_t gid = rules2::uiCreateGroup(name, isOr);
  rules2::saveRules2();
  app.server.sendHeader("Location", String("/config/rules2/group?id=") + gid);
  app.server.send(303);
}

static void handleRules2DeleteGroup() {
  uint32_t id = (uint32_t)app.server.arg("id").toInt();
  rules2::uiDeleteExprNode(id);
  rules2::saveRules2();
  app.server.sendHeader("Location", "/config/rules2/groups");
  app.server.send(303);
}

static void handleRules2SaveGroup() {
  rules2::uiSaveGroupFromPost(app.server);
  rules2::saveRules2();
  uint32_t id = (uint32_t)app.server.arg("id").toInt();
  app.server.sendHeader("Location", String("/config/rules2/group?id=") + id);
  app.server.send(303);
}

static void handleRules2AddChildToGroup() {
  rules2::uiAddChildToGroupFromPost(app.server);
  rules2::saveRules2();
  uint32_t gid = (uint32_t)app.server.arg("gid").toInt();
  app.server.sendHeader("Location", String("/config/rules2/group?id=") + gid);
  app.server.send(303);
}

static void handleRules2RemoveChildFromGroup() {
  rules2::uiRemoveChildFromGroupFromPost(app.server);
  rules2::saveRules2();
  uint32_t gid = (uint32_t)app.server.arg("gid").toInt();
  app.server.sendHeader("Location", String("/config/rules2/group?id=") + gid);
  app.server.send(303);
}



static void handleSaveSettings(Settings& cfg) {
  // --- Control ---
  cfg.tank_sp_c = argFloat("tank_sp_c", cfg.tank_sp_c);
  cfg.tz_offset_min = argInt("tz_offset_min", cfg.tz_offset_min);

  // rtc_epoch can be large; toInt() is 32-bit, but our field is usually safe. If needed we can upgrade.
  if (app.server.hasArg("rtc_epoch")) {
    cfg.rtc_epoch = (int64_t)strtoll(app.server.arg("rtc_epoch").c_str(), nullptr, 10);
  }

  if (app.server.hasArg("ms")) {
    long ms = app.server.arg("ms").toInt();
    if (ms < 10) ms = 10;
    if (ms > 60000) ms = 60000;
    app.blinkMs = (uint32_t)ms;
  }

  // --- MQTT ---
  cfg.mqtt_host = argStr("mqtt_host", cfg.mqtt_host);
  cfg.mqtt_port = argInt("mqtt_port", cfg.mqtt_port);
  cfg.mqtt_user = argStr("mqtt_user", cfg.mqtt_user);
  cfg.mqtt_pass = argStr("mqtt_pass", cfg.mqtt_pass);
  cfg.mqtt_base = argStr("mqtt_base", cfg.mqtt_base);

  // --- Wi-Fi ---
  if (app.server.hasArg("wifi_ssid")) cfg.wifi_ssid = app.server.arg("wifi_ssid");
  if (app.server.hasArg("wifi_pass")) cfg.wifi_pass = app.server.arg("wifi_pass");
  if (app.server.hasArg("ap_pass"))   cfg.ap_pass   = app.server.arg("ap_pass");

  // --- Shunts ---
  cfg.shunt_mode = argStr("shunt_mode", cfg.shunt_mode);

  cfg.pv_shunt_a = argInt("pv_shunt_a", cfg.pv_shunt_a);
  cfg.pv_shunt_mv = argInt("pv_shunt_mv", cfg.pv_shunt_mv);
  cfg.pv_shunt_mohm = argFloat("pv_shunt_mohm", cfg.pv_shunt_mohm);

  cfg.main_shunt_a = argInt("main_shunt_a", cfg.main_shunt_a);
  cfg.main_shunt_mv = argInt("main_shunt_mv", cfg.main_shunt_mv);
  cfg.main_shunt_mohm = argFloat("main_shunt_mohm", cfg.main_shunt_mohm);

  // --- Relays ---
  cfg.div_loc = argStr("div_loc", cfg.div_loc);
  cfg.div_idx = argInt("div_idx", cfg.div_idx);
  cfg.pvkill_loc = argStr("pvkill_loc", cfg.pvkill_loc);
  cfg.pvkill_idx = argInt("pvkill_idx", cfg.pvkill_idx);

  // --- PV model ---
  cfg.pv_ns = argInt("pv_ns", cfg.pv_ns);
  cfg.pv_np = argInt("pv_np", cfg.pv_np);
  cfg.pv_vmp = argFloat("pv_vmp", cfg.pv_vmp);
  cfg.pv_voc = argFloat("pv_voc", cfg.pv_voc);
  cfg.pv_imp = argFloat("pv_imp", cfg.pv_imp);

  // --- Elements ---
  for (int i = 0; i < 8; i++) {
    String kv = "elv" + String(i);
    String kw = "elw" + String(i);
    if (app.server.hasArg(kv)) cfg.el_v[i] = app.server.arg(kv).toFloat();
    if (app.server.hasArg(kw)) cfg.el_w[i] = app.server.arg(kw).toFloat();
  }
  cfg.learn_elems = app.server.hasArg("learn_elems");

  validateSettings(cfg);
  saveSettings(cfg);






  // redirect target
  String ret = app.server.hasArg("return") ? app.server.arg("return") : String("/config");

  // In AP mode, saving Wi-Fi should reboot to attempt STA join
  if (app.inApMode && (app.server.hasArg("wifi_ssid") || app.server.hasArg("wifi_pass"))) {
    app.server.send(200, "text/plain", "Saved. Rebooting to connect to Wi-Fi...");
    delay(250);
    ESP.restart();
    return;
  }

  app.server.sendHeader("Location", ret);
  app.server.send(303);
}


static void handleForgetWiFi() {
  app.prefs.remove("wifi_ssid");
  app.prefs.remove("wifi_pass");
  app.server.send(200, "text/plain", "Cleared Wi-Fi. Rebooting into AP setup...");
  delay(250);
  ESP.restart();
}

static void handleReboot() {
  app.server.send(200, "text/plain", "Rebooting...");
  delay(250);
  ESP.restart();
}

void registerWebRoutes(Settings& cfg) {
  // Wrap handlers that need cfg
  app.server.on("/", HTTP_GET, [&cfg](){ handleMain(cfg); });
  app.server.on("/saveSettings", HTTP_POST, [&cfg](){ handleSaveSettings(cfg); });

  app.server.on("/rules", HTTP_GET, handleRules);
  app.server.on("/saveRules", HTTP_POST, handleSaveRules);

  app.server.on("/logo.png", HTTP_GET, handleLogo);
  app.server.on("/forgetWiFi", HTTP_POST, handleForgetWiFi);
  app.server.on("/reboot", HTTP_POST, handleReboot);

  app.server.on("/config", HTTP_GET, [&cfg](){
    app.server.send(200, "text/html", buildConfigHomeHtml(cfg, app.inApMode));
  });

  app.server.on("/config/control", HTTP_GET, [&cfg](){
    app.server.send(200, "text/html", buildConfigControlHtml(cfg));
  });
  app.server.on("/config/mqtt", HTTP_GET, [&cfg](){
    app.server.send(200, "text/html", buildConfigMqttHtml(cfg));
  });
  app.server.on("/config/wifi", HTTP_GET, [&cfg](){
    app.server.send(200, "text/html", buildConfigWifiHtml(cfg, app.inApMode));
  });
  app.server.on("/config/shunts", HTTP_GET, [&cfg](){
    app.server.send(200, "text/html", buildConfigShuntsHtml(cfg));
  });
  app.server.on("/config/relays", HTTP_GET, [&cfg](){
    app.server.send(200, "text/html", buildConfigRelaysHtml(cfg));
  });
  app.server.on("/config/pv", HTTP_GET, [&cfg](){
    app.server.send(200, "text/html", buildConfigPvHtml(cfg));
  });
  app.server.on("/config/elements", HTTP_GET, [&cfg](){
    app.server.send(200, "text/html", buildConfigElementsHtml(cfg));
  });

  // --- Rules v2 (parallel) ---
  app.server.on("/config/rules2", HTTP_GET, [&cfg](){ handleRules2(cfg); });
  app.server.on("/config/rules2/new", HTTP_POST, handleRules2NewRule);
  app.server.on("/config/rules2/edit", HTTP_GET, [&cfg](){ handleRules2EditRule(cfg); });
  app.server.on("/config/rules2/save", HTTP_POST, handleRules2SaveRule);
  app.server.on("/config/rules2/delete", HTTP_POST, handleRules2DeleteRule);

  app.server.on("/config/rules2/conditions", HTTP_GET, [&cfg](){ handleRules2Conditions(cfg); });
  app.server.on("/config/rules2/conditions/new", HTTP_POST, handleRules2NewCondition);
  app.server.on("/config/rules2/conditions/save", HTTP_POST, handleRules2SaveConditions);
  app.server.on("/config/rules2/conditions/delete", HTTP_POST, handleRules2DeleteCondition);

    // --- Rules v2 groups ---
  app.server.on("/config/rules2/groups", HTTP_GET, [&cfg](){ handleRules2Groups(cfg); });
  app.server.on("/config/rules2/groups/new", HTTP_POST, handleRules2NewGroup);
  app.server.on("/config/rules2/groups/delete", HTTP_POST, handleRules2DeleteGroup);

  app.server.on("/config/rules2/group", HTTP_GET, [&cfg](){ handleRules2EditGroup(cfg); });
  app.server.on("/config/rules2/group/save", HTTP_POST, handleRules2SaveGroup);
  app.server.on("/config/rules2/group/addChild", HTTP_POST, handleRules2AddChildToGroup);
  app.server.on("/config/rules2/group/removeChild", HTTP_POST, handleRules2RemoveChildFromGroup);

  app.server.on("/config/rules2/demo", HTTP_POST, handleRules2Demo);

  app.server.onNotFound([]() {
    app.server.send(404, "text/plain", "Not Found");
  });
}
