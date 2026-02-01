#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <Preferences.h>

struct App {
  WebServer server{80};
  Preferences prefs;

  bool inApMode = false;
  String apSsid;

  // optional heartbeat/debug
  uint32_t blinkMs = 500;
};

extern App app;
