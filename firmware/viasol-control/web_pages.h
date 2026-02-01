#pragma once
#include <Arduino.h>
#include "settings.h"

String buildMainHtml(const Settings& cfg, const String& ipStr, bool inApMode);

// Config home + subpages
String buildConfigHomeHtml(const Settings& cfg, bool inApMode);
String buildConfigControlHtml(const Settings& cfg);
String buildConfigMqttHtml(const Settings& cfg);
String buildConfigWifiHtml(const Settings& cfg, bool inApMode);
String buildConfigShuntsHtml(const Settings& cfg);
String buildConfigRelaysHtml(const Settings& cfg);
String buildConfigPvHtml(const Settings& cfg);
String buildConfigElementsHtml(const Settings& cfg);

String buildRulesHtml();
