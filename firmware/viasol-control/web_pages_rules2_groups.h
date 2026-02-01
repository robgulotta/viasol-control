#pragma once
#include <Arduino.h>
#include "settings.h"

String buildRules2GroupsHtml(Settings& cfg);
String buildRules2EditGroupHtml(Settings& cfg, uint32_t groupId);
