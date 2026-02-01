#include "rules.h"
#include "app.h"
#include "io_catalog.h"
#include <Preferences.h>

struct RuleRuntime {
  bool lastCondition = false;
  bool active = false;
  uint32_t activeUntilMs = 0;
};

Rule rules[MAX_RULES];
static RuleRuntime rr[MAX_RULES];

const char* opToStr(CmpOp op) {
  switch(op){
    case CmpOp::GT: return ">";
    case CmpOp::GE: return ">=";
    case CmpOp::LT: return "<";
    case CmpOp::LE: return "<=";
    case CmpOp::EQ: return "==";
    case CmpOp::NE: return "!=";
  }
  return ">";
}

CmpOp strToOp(const String& s) {
  if (s == ">")  return CmpOp::GT;
  if (s == ">=") return CmpOp::GE;
  if (s == "<")  return CmpOp::LT;
  if (s == "<=") return CmpOp::LE;
  if (s == "==") return CmpOp::EQ;
  if (s == "!=") return CmpOp::NE;
  return CmpOp::GT;
}

const char* modeToStr(RuleMode m) {
  switch(m){
    case RuleMode::FOLLOW: return "follow";
    case RuleMode::ONCE:   return "once";
    case RuleMode::TIMED:  return "timed";
  }
  return "follow";
}

RuleMode strToMode(const String& s) {
  if (s == "follow") return RuleMode::FOLLOW;
  if (s == "once")   return RuleMode::ONCE;
  if (s == "timed")  return RuleMode::TIMED;
  return RuleMode::FOLLOW;
}

const char* rhsToStr(RhsSource r) {
  return (r == RhsSource::INPUT_KEY) ? "input" : "const";
}

RhsSource strToRhs(const String& s) {
  return (s == "input") ? RhsSource::INPUT_KEY : RhsSource::CONST_VAL;
}

static bool evalCmp(float x, CmpOp op, float th) {
  switch(op){
    case CmpOp::GT: return x >  th;
    case CmpOp::GE: return x >= th;
    case CmpOp::LT: return x <  th;
    case CmpOp::LE: return x <= th;
    case CmpOp::EQ: return x == th;
    case CmpOp::NE: return x != th;
  }
  return false;
}

void loadRules() {
  for (int i = 0; i < MAX_RULES; i++) {
    char k[24];

    snprintf(k, sizeof(k), "r%d_en", i);
    rules[i].enabled = app.prefs.getBool(k, false);

    snprintf(k, sizeof(k), "r%d_in", i);
    rules[i].inputKey = app.prefs.getString(k, "tank_temp_c");

    snprintf(k, sizeof(k), "r%d_op", i);
    rules[i].op = strToOp(app.prefs.getString(k, ">"));

    snprintf(k, sizeof(k), "r%d_rhs", i);
    rules[i].rhsSource = strToRhs(app.prefs.getString(k, "const"));

    snprintf(k, sizeof(k), "r%d_th", i);
    rules[i].threshold = app.prefs.getFloat(k, 0.0f);

    snprintf(k, sizeof(k), "r%d_rin", i);
    rules[i].rhsInputKey = app.prefs.getString(k, "tank_temp_c");

    snprintf(k, sizeof(k), "r%d_out", i);
    rules[i].outputKey = app.prefs.getString(k, "m_relay1");

    snprintf(k, sizeof(k), "r%d_on", i);
    rules[i].outputOn = app.prefs.getBool(k, true);

    snprintf(k, sizeof(k), "r%d_mode", i);
    rules[i].mode = strToMode(app.prefs.getString(k, "follow"));

    snprintf(k, sizeof(k), "r%d_dur", i);
    rules[i].durationSec = (uint32_t)app.prefs.getUInt(k, 0);

    rr[i] = RuleRuntime{};
  }
}

void saveRules() {
  for (int i = 0; i < MAX_RULES; i++) {
    char k[24];

    snprintf(k, sizeof(k), "r%d_en", i);
    app.prefs.putBool(k, rules[i].enabled);

    snprintf(k, sizeof(k), "r%d_in", i);
    app.prefs.putString(k, rules[i].inputKey);

    snprintf(k, sizeof(k), "r%d_op", i);
    app.prefs.putString(k, opToStr(rules[i].op));

    snprintf(k, sizeof(k), "r%d_rhs", i);
    app.prefs.putString(k, rhsToStr(rules[i].rhsSource));

    snprintf(k, sizeof(k), "r%d_th", i);
    app.prefs.putFloat(k, rules[i].threshold);

    snprintf(k, sizeof(k), "r%d_rin", i);
    app.prefs.putString(k, rules[i].rhsInputKey);

    snprintf(k, sizeof(k), "r%d_out", i);
    app.prefs.putString(k, rules[i].outputKey);

    snprintf(k, sizeof(k), "r%d_on", i);
    app.prefs.putBool(k, rules[i].outputOn);

    snprintf(k, sizeof(k), "r%d_mode", i);
    app.prefs.putString(k, modeToStr(rules[i].mode));

    snprintf(k, sizeof(k), "r%d_dur", i);
    app.prefs.putUInt(k, rules[i].durationSec);
  }
}

void processRules() {
  uint32_t now = millis();

  for (int i = 0; i < MAX_RULES; i++) {
    if (!rules[i].enabled) continue;

    float lhs = inputValueByKey(rules[i].inputKey);
    float rhs = rules[i].threshold;
    if (rules[i].rhsSource == RhsSource::INPUT_KEY) {
      rhs = inputValueByKey(rules[i].rhsInputKey);
    }

    bool cond = evalCmp(lhs, rules[i].op, rhs);
    bool rising = (cond && !rr[i].lastCondition);
    rr[i].lastCondition = cond;

    switch (rules[i].mode) {
      case RuleMode::FOLLOW: {
        bool drive = cond ? rules[i].outputOn : !rules[i].outputOn;
        applyOutput(rules[i].outputKey, drive);
      } break;

      case RuleMode::ONCE: {
        if (rising) {
          uint32_t dur = (rules[i].durationSec == 0) ? 1 : rules[i].durationSec;
          rr[i].active = true;
          rr[i].activeUntilMs = now + dur * 1000UL;
          applyOutput(rules[i].outputKey, rules[i].outputOn);
        }
        if (rr[i].active && (int32_t)(now - rr[i].activeUntilMs) >= 0) {
          rr[i].active = false;
          applyOutput(rules[i].outputKey, !rules[i].outputOn);
        }
      } break;

      case RuleMode::TIMED: {
        if (rising) {
          uint32_t dur = (rules[i].durationSec == 0) ? 1 : rules[i].durationSec;
          rr[i].active = true;
          rr[i].activeUntilMs = now + dur * 1000UL;
          applyOutput(rules[i].outputKey, rules[i].outputOn);
        }
        if (rr[i].active && (int32_t)(now - rr[i].activeUntilMs) >= 0) {
          rr[i].active = false;
          applyOutput(rules[i].outputKey, !rules[i].outputOn);
        }
      } break;
    }
  }
}
