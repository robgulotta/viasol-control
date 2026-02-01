#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>

#include "app.h"
#include "settings.h"
#include "rules.h"
#include "web_routes.h"
#include "rules2.h"



// Global settings (as requested)
Settings cfg;


static String chipSuffix() {
  uint64_t mac = ESP.getEfuseMac();
  uint32_t low = (uint32_t)(mac & 0xFFFFFFFF);
  char buf[9];
  snprintf(buf, sizeof(buf), "%08X", (unsigned)low);
  return String(buf).substring(2);
}

static bool connectToSta(uint32_t timeoutMs) {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.disconnect(true, true);
  delay(200);

  Serial.printf("Connecting to WiFi SSID: '%s'\n", cfg.wifi_ssid.c_str());
  WiFi.begin(cfg.wifi_ssid.c_str(), cfg.wifi_pass.c_str());

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeoutMs) {
    delay(300);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected. IP: ");
    Serial.println(WiFi.localIP());
    return true;
  }

  Serial.println("WiFi connect failed.");
  return false;
}

static void startApMode() {
  app.inApMode = true;
  WiFi.mode(WIFI_AP);

  bool ok;
  if (cfg.ap_pass.length() >= 8) ok = WiFi.softAP(app.apSsid.c_str(), cfg.ap_pass.c_str());
  else ok = WiFi.softAP(app.apSsid.c_str());

  Serial.printf("AP start %s\n", ok ? "OK" : "FAILED");
  Serial.print("AP SSID: "); Serial.println(app.apSsid);
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());
}

void setup() {
  Serial.begin(115200);
  delay(200);

  // LittleFS for logo and later assets
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed");
  }

  // Unique AP SSID
  app.apSsid = "ViaSol-" + chipSuffix();

  // Load settings (this also begins prefs)
  loadSettings(cfg);

  // Load programmable rules
  loadRules();

  // Try STA first if creds exist, else AP mode
  bool connected = false;
  if (cfg.wifi_ssid.length() && cfg.wifi_pass.length()) {
    connected = connectToSta(15000);
  }
  if (!connected) startApMode();
  else app.inApMode = false;

  // Register routes and start server
  registerWebRoutes(cfg);
  app.server.begin();

  rules2::loadRules2();          // or initRules2Defaults() for now
  //rules2::initRules2Defaults();   // temporary until UI/persistence exists


  Serial.println("Web server started.");
}

void loop() {
  app.server.handleClient();
  processRules();
  rules2::processRules2(); // new rules v2 (parallel)



  // Optional: simple heartbeat blink without touching outputs map yet
  static uint32_t lastToggle = 0;
  static bool ledState = false;
#ifndef LED_BUILTIN
  #define LED_BUILTIN 2
#endif
  const int LED_PIN = LED_BUILTIN;
  pinMode(LED_PIN, OUTPUT);

  uint32_t now = millis();
  if (now - lastToggle >= app.blinkMs) {
    lastToggle = now;
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
  }

  // Later: heaterTick(); mqttTick(); rs485Tick(); etc.
}
