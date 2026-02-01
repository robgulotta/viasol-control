#pragma once
#include <Arduino.h>
#include "settings.h"

String buildRules2HomeHtml(Settings& cfg);
String buildRules2ConditionsHtml(Settings& cfg);
String buildRules2EditRuleHtml(Settings& cfg, uint32_t ruleId);
