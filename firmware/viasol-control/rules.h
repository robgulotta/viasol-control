#pragma once
#include <Arduino.h>

enum class CmpOp : uint8_t { GT, GE, LT, LE, EQ, NE };
enum class RuleMode : uint8_t { FOLLOW, ONCE, TIMED };
enum class RhsSource : uint8_t { CONST_VAL, INPUT_KEY };

struct Rule {
  bool enabled = false;
  String inputKey = "tank_temp_c";
  CmpOp op = CmpOp::GT;

  RhsSource rhsSource = RhsSource::CONST_VAL;
  float threshold = 0.0f;
  String rhsInputKey = "tank_temp_c";

  String outputKey = "m_relay1";
  bool outputOn = true;

  RuleMode mode = RuleMode::FOLLOW;
  uint32_t durationSec = 0;
};

static const int MAX_RULES = 12;

extern Rule rules[MAX_RULES];

const char* opToStr(CmpOp op);
CmpOp strToOp(const String& s);

const char* modeToStr(RuleMode m);
RuleMode strToMode(const String& s);

const char* rhsToStr(RhsSource r);
RhsSource strToRhs(const String& s);

void loadRules();
void saveRules();
void processRules();
