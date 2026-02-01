#include "rules2.h"
#include "io_catalog.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

namespace rules2 {

// -----------------------------------------------------------------------------
// Global DB
// -----------------------------------------------------------------------------
Db db;


static const char* RULES2_PATH = "/rules2.json";
static const uint16_t RULES2_SCHEMA = 1;


// -----------------------------------------------------------------------------
// Internal helpers
// -----------------------------------------------------------------------------
static bool cmp(CmpOp op, float a, float b) {
  switch (op) {
    case CmpOp::GT: return a >  b;
    case CmpOp::GE: return a >= b;
    case CmpOp::LT: return a <  b;
    case CmpOp::LE: return a <= b;
    case CmpOp::EQ: return a == b;
    case CmpOp::NE: return a != b;
  }
  return false;
}

static CmpOp strToOp2(const String& s) {
  if (s == "GT") return CmpOp::GT;
  if (s == "GE") return CmpOp::GE;
  if (s == "LT") return CmpOp::LT;
  if (s == "LE") return CmpOp::LE;
  if (s == "EQ") return CmpOp::EQ;
  if (s == "NE") return CmpOp::NE;
  return CmpOp::GT;
}

// -----------------------------------------------------------------------------
// Db helpers
// -----------------------------------------------------------------------------
Condition* Db::findCond(uint32_t id) {
  for (auto& c : conditions) if (c.id == id) return &c;
  return nullptr;
}

ExprNode* Db::findExpr(uint32_t id) {
  for (auto& e : expr) if (e.id == id) return &e;
  return nullptr;
}

Rule* Db::findRule(uint32_t id) {
  for (auto& r : rules) if (r.id == id) return &r;
  return nullptr;
}

// -----------------------------------------------------------------------------
// Condition evaluation
// -----------------------------------------------------------------------------
bool evalCondition(Condition& c, uint32_t nowMs) {
  if (!c.enabled) return false;

  bool raw = false;
  float lhs = inputValueByKey(c.inputKey);

  if (c.type == CondType::CompareInputToConst) {
    raw = cmp(c.op, lhs, c.threshold);
  } else { // CompareInputToInput
    float rhs = inputValueByKey(c.rhsInputKey);
    raw = cmp(c.op, lhs, rhs);
  }

  // Stability handling
  if (c.stableForMs == 0) {
    c.lastEval = raw;
    return raw;
  }

  if (raw != c.lastEval) {
    c.lastEval = raw;
    c.lastFlipMs = nowMs;
  }

  if (!raw) return false;
  return (nowMs - c.lastFlipMs) >= c.stableForMs;
}

// -----------------------------------------------------------------------------
// Expression evaluation
// -----------------------------------------------------------------------------
bool evalExpr(uint32_t exprId, uint32_t nowMs) {
  ExprNode* n = db.findExpr(exprId);
  if (!n) return false;

  switch (n->type) {
    case ExprType::LeafCond: {
      Condition* c = db.findCond(n->condId);
      return c ? evalCondition(*c, nowMs) : false;
    }

    case ExprType::Not:
      return !evalExpr(n->child, nowMs);

    case ExprType::And:
      if (n->children.empty()) return true;
      for (uint32_t cid : n->children) {
        if (!evalExpr(cid, nowMs)) return false;
      }
      return true;

    case ExprType::Or:
      if (n->children.empty()) return false;
      for (uint32_t cid : n->children) {
        if (evalExpr(cid, nowMs)) return true;
      }
      return false;
  }
  return false;
}

// -----------------------------------------------------------------------------
// Action application (simple hold logic for durationMs)
// -----------------------------------------------------------------------------
struct Hold {
  String key;
  uint32_t untilMs;
};

static std::vector<Hold> holds;

static void serviceHolds(uint32_t nowMs) {
  for (int i = (int)holds.size() - 1; i >= 0; --i) {
    if (nowMs >= holds[i].untilMs) {
      applyOutput(holds[i].key, false);
      holds.erase(holds.begin() + i);
    }
  }
}

static void applyActions(const Rule& r, uint32_t nowMs) {
  for (const auto& a : r.actions) {
    if (a.type != ActionType::SetOutput) continue;

    applyOutput(a.outputKey, a.on);

    if (a.on && a.durationMs > 0) {
      Hold h;
      h.key = a.outputKey;
      h.untilMs = nowMs + a.durationMs;
      holds.push_back(h);
    }
  }
}

// -----------------------------------------------------------------------------
// Rules engine tick
// -----------------------------------------------------------------------------
void processRules2() {
  uint32_t nowMs = millis();

  serviceHolds(nowMs);

  for (auto& r : db.rules) {
    if (!r.enabled) continue;

    if (r.minEvalPeriodMs > 0 && (nowMs - r.lastEvalMs) < r.minEvalPeriodMs) {
      continue;
    }
    r.lastEvalMs = nowMs;

    bool result = evalExpr(r.exprRootId, nowMs);

    // edge-trigger: false -> true
    bool rising = (result && !r.lastResult);
    r.lastResult = result;
    if (!rising) continue;

    // cooldown
    if (r.cooldownMs > 0 && (nowMs - r.lastTriggerMs) < r.cooldownMs) {
      continue;
    }

    r.lastTriggerMs = nowMs;
    applyActions(r, nowMs);
  }
}

// -----------------------------------------------------------------------------
// UI helpers (conditions)
// -----------------------------------------------------------------------------
uint32_t uiCreateDefaultCondition() {
  Condition c;
  c.id = db.allocId();
  c.enabled = true;
  c.name = "New condition";

  c.type = CondType::CompareInputToConst;
  c.inputKey = (N_INPUTS > 0) ? INPUT_KEYS[0] : "tank_temp_c";
  c.op = CmpOp::GT;

  c.threshold = 0.0f;
  c.rhsInputKey = (N_INPUTS > 0) ? INPUT_KEYS[0] : "tank_temp_c";
  c.stableForMs = 0;

  db.conditions.push_back(c);
  return c.id;
}

void uiDeleteCondition(uint32_t id) {
  for (int i = (int)db.conditions.size() - 1; i >= 0; --i) {
    if (db.conditions[i].id == id) {
      db.conditions.erase(db.conditions.begin() + i);
    }
  }
}

void uiSaveConditionsFromPost(WebServer& s) {
  for (auto& c : db.conditions) {
    String base = "c" + String(c.id) + "_";

    c.enabled = s.hasArg(base + "en");
    if (s.hasArg(base + "name")) c.name = s.arg(base + "name");
    if (s.hasArg(base + "in"))   c.inputKey = s.arg(base + "in");
    if (s.hasArg(base + "op"))   c.op = strToOp2(s.arg(base + "op"));

    // RHS selector: CONST or INPUT
    String rhs = s.hasArg(base + "rhs") ? s.arg(base + "rhs") : "CONST";
    if (rhs == "INPUT") {
      c.type = CondType::CompareInputToInput;
      if (s.hasArg(base + "rin")) c.rhsInputKey = s.arg(base + "rin");
    } else {
      c.type = CondType::CompareInputToConst;
      if (s.hasArg(base + "th")) c.threshold = s.arg(base + "th").toFloat();
    }

    if (s.hasArg(base + "st")) c.stableForMs = (uint32_t)s.arg(base + "st").toInt();
  }
}

// -----------------------------------------------------------------------------
// Rules (now select root group / expr)
// -----------------------------------------------------------------------------
uint32_t uiCreateDefaultRule() {
  if (db.conditions.empty()) uiCreateDefaultCondition();

  // Default: leaf expression referencing first condition (so rule has something usable)
  ExprNode leaf;
  leaf.id = db.allocId();
  leaf.type = ExprType::LeafCond;
  leaf.condId = db.conditions[0].id;
  leaf.name = "leaf";
  db.expr.push_back(leaf);

  Rule r;
  r.id = db.allocId();
  r.enabled = true;
  r.name = "New rule";
  r.exprRootId = leaf.id;
  r.minEvalPeriodMs = 250;
  r.cooldownMs = 0;

  Action a;
  a.type = ActionType::SetOutput;
  a.outputKey = (N_OUTPUTS > 0) ? OUTPUT_KEYS[0] : "m_relay1";
  a.on = true;
  a.durationMs = 0;
  r.actions.push_back(a);

  db.rules.push_back(r);
  return r.id;
}

void uiDeleteRule(uint32_t id) {
  for (int i = (int)db.rules.size() - 1; i >= 0; --i) {
    if (db.rules[i].id == id) db.rules.erase(db.rules.begin() + i);
  }
}

// Save rule from POST:
// - If user selected a root expr/group -> use it
// - Else, fallback to old MVP checkbox builder (flat AND/OR) (still useful)
// - Safety: if nothing selected at all -> disable rule and exprRootId=0
void uiSaveRuleFromPost(WebServer& s) {
  if (!s.hasArg("id")) return;
  uint32_t id = (uint32_t)s.arg("id").toInt();

  Rule* r = db.findRule(id);
  if (!r) return;

  if (s.hasArg("name")) r->name = s.arg("name");
  r->enabled = s.hasArg("en");



  if (s.hasArg("minEval")) r->minEvalPeriodMs = (uint32_t)s.arg("minEval").toInt();
  if (s.hasArg("cooldown")) r->cooldownMs = (uint32_t)s.arg("cooldown").toInt();
  if (s.hasArg("priority")) r->priority = (int16_t)s.arg("priority").toInt();


  // 1) Preferred: choose an existing expression/group as the root
  if (s.hasArg("rootExpr")) {
    uint32_t rootId = (uint32_t)s.arg("rootExpr").toInt();
    // rootExpr=0 means "none"
    if (rootId != 0 && db.findExpr(rootId)) {
      r->exprRootId = rootId;
      // keep enabled as user set it
    } else if (rootId == 0) {
      // explicitly none: disable for safety
      r->enabled = false;
      r->exprRootId = 0;
    }
  }

  // 2) If user used checkbox builder, it posts cond checkboxes.
  // Collect selected condition IDs (checkboxes named 'cond')
  std::vector<uint32_t> selected;
  for (int i = 0; i < s.args(); i++) {
    if (s.argName(i) == "cond") {
      selected.push_back((uint32_t)s.arg(i).toInt());
    }
  }

  // If user provided cond selections, rebuild a flat group and set as root
  if (!selected.empty()) {
    std::vector<uint32_t> leaves;
    for (uint32_t cid : selected) {
      ExprNode leaf;
      leaf.id = db.allocId();
      leaf.type = ExprType::LeafCond;
      leaf.condId = cid;
      leaf.name = "leaf";
      db.expr.push_back(leaf);
      leaves.push_back(leaf.id);
    }

    ExprNode root;
    root.id = db.allocId();
    String mode = s.hasArg("exprMode") ? s.arg("exprMode") : "AND";
    root.type = (mode == "OR") ? ExprType::Or : ExprType::And;
    root.name = "MVP group";
    root.children = leaves;
    db.expr.push_back(root);

    r->exprRootId = root.id;
  }

  // Safety: If rule has no valid expr root, disable it
  if (r->exprRootId == 0 || !db.findExpr(r->exprRootId)) {
    r->enabled = false;
    r->exprRootId = 0;
  }

  // Save single action (MVP)
  if (r->actions.empty()) r->actions.push_back(Action{});
  Action& a = r->actions[0];

  if (s.hasArg("out")) a.outputKey = s.arg("out");
  if (s.hasArg("on"))  a.on = (s.arg("on").toInt() != 0);
  if (s.hasArg("dur")) a.durationMs = (uint32_t)s.arg("dur").toInt();
}

// -----------------------------------------------------------------------------
// Groups (ExprNode) helpers
// -----------------------------------------------------------------------------
uint32_t uiCreateGroup(const String& name, bool isOr) {
  ExprNode g;
  g.id = db.allocId();
  g.type = isOr ? ExprType::Or : ExprType::And;
  g.name = name.length() ? name : String(isOr ? "New OR group" : "New AND group");
  db.expr.push_back(g);
  return g.id;
}

void uiDeleteExprNode(uint32_t exprId) {
  // Basic delete: remove the node from db.expr, and remove references from any group children lists.
  for (auto& e : db.expr) {
    if (e.type == ExprType::And || e.type == ExprType::Or) {
      for (int i = (int)e.children.size() - 1; i >= 0; --i) {
        if (e.children[i] == exprId) e.children.erase(e.children.begin() + i);
      }
    }
    if (e.type == ExprType::Not && e.child == exprId) {
      e.child = 0;
    }
  }

  for (int i = (int)db.expr.size() - 1; i >= 0; --i) {
    if (db.expr[i].id == exprId) db.expr.erase(db.expr.begin() + i);
  }

  // Also detach rules that reference it
  for (auto& r : db.rules) {
    if (r.exprRootId == exprId) {
      r.exprRootId = 0;
      r.enabled = false;
    }
  }
}

void uiSaveGroupFromPost(WebServer& s) {
  if (!s.hasArg("id")) return;
  uint32_t id = (uint32_t)s.arg("id").toInt();
  ExprNode* g = db.findExpr(id);
  if (!g) return;

  if (s.hasArg("name")) g->name = s.arg("name");

  if (s.hasArg("gtype")) {
    String t = s.arg("gtype");
    if (t == "OR") g->type = ExprType::Or;
    else if (t == "AND") g->type = ExprType::And;
  }
}

static uint32_t createLeafForCond(uint32_t condId) {
  ExprNode leaf;
  leaf.id = db.allocId();
  leaf.type = ExprType::LeafCond;
  leaf.condId = condId;
  leaf.name = "leaf";
  db.expr.push_back(leaf);
  return leaf.id;
}

void uiAddChildToGroupFromPost(WebServer& s) {
  if (!s.hasArg("gid")) return;
  uint32_t gid = (uint32_t)s.arg("gid").toInt();
  ExprNode* g = db.findExpr(gid);
  if (!g) return;
  if (!(g->type == ExprType::And || g->type == ExprType::Or)) return;

  // Add a condition as a new leaf
  if (s.hasArg("addCond")) {
    uint32_t cid = (uint32_t)s.arg("addCond").toInt();
    if (db.findCond(cid)) {
      uint32_t leafId = createLeafForCond(cid);
      g->children.push_back(leafId);
    }
  }

  // Add an existing group as a child (nesting)
  if (s.hasArg("addGroup")) {
    uint32_t childId = (uint32_t)s.arg("addGroup").toInt();
    if (childId != 0 && childId != gid && db.findExpr(childId)) {
      // Basic cycle protection: disallow self
      g->children.push_back(childId);
    }
  }
}

void uiRemoveChildFromGroupFromPost(WebServer& s) {
  if (!s.hasArg("gid") || !s.hasArg("idx")) return;
  uint32_t gid = (uint32_t)s.arg("gid").toInt();
  int idx = s.arg("idx").toInt();

  ExprNode* g = db.findExpr(gid);
  if (!g) return;
  if (!(g->type == ExprType::And || g->type == ExprType::Or)) return;

  if (idx < 0 || idx >= (int)g->children.size()) return;
  g->children.erase(g->children.begin() + idx);
}

// -----------------------------------------------------------------------------
// Introspection helpers
// -----------------------------------------------------------------------------
static void collectLeafCondIds(uint32_t exprId, std::vector<uint32_t>& out) {
  ExprNode* n = db.findExpr(exprId);
  if (!n) return;

  if (n->type == ExprType::LeafCond) {
    out.push_back(n->condId);
    return;
  }
  if (n->type == ExprType::Not) {
    collectLeafCondIds(n->child, out);
    return;
  }
  for (uint32_t childId : n->children) {
    collectLeafCondIds(childId, out);
  }
}

void getRuleSelectedCondIds(uint32_t ruleId, std::vector<uint32_t>& out) {
  out.clear();
  Rule* r = db.findRule(ruleId);
  if (!r) return;
  collectLeafCondIds(r->exprRootId, out);
}

const char* getRuleExprModeStr(uint32_t ruleId) {
  Rule* r = db.findRule(ruleId);
  if (!r) return "AND";
  ExprNode* root = db.findExpr(r->exprRootId);
  if (!root) return "AND";
  return (root->type == ExprType::Or) ? "OR" : "AND";
}

String describeExpr(uint32_t exprId) {
  ExprNode* e = db.findExpr(exprId);
  if (!e) return "none";
  if (e->type == ExprType::And || e->type == ExprType::Or) {
    String t = (e->type == ExprType::Or) ? "OR" : "AND";
    String nm = e->name.length() ? e->name : String("group ") + e->id;
    return nm + " [" + t + "] (" + String((int)e->children.size()) + ")";
  }
  if (e->type == ExprType::LeafCond) {
    Condition* c = db.findCond(e->condId);
    if (!c) return String("leaf(cond ") + e->condId + ")";
    String nm = c->name.length() ? c->name : String("cond ") + c->id;
    return "leaf: " + nm;
  }
  if (e->type == ExprType::Not) {
    return "NOT(...)";
  }
  return "expr";
}

// -----------------------------------------------------------------------------
// Persistence stubs
// -----------------------------------------------------------------------------
void saveRules2() {
  // Bump this if you add more fields or have lots of rules/expr/conds.
  // ESP32-S3 should handle 48KB fine.
  DynamicJsonDocument doc(49152);

  doc["schema"] = RULES2_SCHEMA;
  doc["nextId"] = db.nextId;

  // ---------------------------------------------------------------------------
  // Conditions
  // ---------------------------------------------------------------------------
  JsonArray conds = doc.createNestedArray("conditions");
  for (const auto& c : db.conditions) {
    JsonObject o = conds.createNestedObject();
    o["id"] = c.id;
    o["enabled"] = c.enabled;
    o["name"] = c.name;

    o["type"] = (uint8_t)c.type;
    o["inputKey"] = c.inputKey;
    o["op"] = (uint8_t)c.op;

    o["threshold"] = c.threshold;
    o["rhsInputKey"] = c.rhsInputKey;

    o["stableForMs"] = c.stableForMs;
  }

  // ---------------------------------------------------------------------------
  // Expr nodes (groups / tree)
  // ---------------------------------------------------------------------------
  JsonArray expr = doc.createNestedArray("expr");
  for (const auto& e : db.expr) {
    JsonObject o = expr.createNestedObject();
    o["id"] = e.id;
    o["type"] = (uint8_t)e.type;
    o["name"] = e.name;

    o["condId"] = e.condId;
    o["child"] = e.child;

    JsonArray kids = o.createNestedArray("children");
    for (uint32_t cid : e.children) kids.add(cid);
  }

  // ---------------------------------------------------------------------------
  // Rules + actions
  // ---------------------------------------------------------------------------
  JsonArray rules = doc.createNestedArray("rules");
  for (const auto& r : db.rules) {
    JsonObject o = rules.createNestedObject();
    o["id"] = r.id;
    o["priority"] = r.priority;   // lower = higher priority (0 is top)
    o["enabled"] = r.enabled;
    o["name"] = r.name;

    o["exprRootId"] = r.exprRootId;
    o["minEvalPeriodMs"] = r.minEvalPeriodMs;
    o["cooldownMs"] = r.cooldownMs;

    JsonArray acts = o.createNestedArray("actions");
    for (const auto& a : r.actions) {
      JsonObject ao = acts.createNestedObject();
      ao["type"] = (uint8_t)a.type;
      ao["outputKey"] = a.outputKey;
      ao["on"] = a.on;
      ao["durationMs"] = a.durationMs;
    }
  }

  // ---------------------------------------------------------------------------
  // Write file
  // ---------------------------------------------------------------------------
  File f = LittleFS.open(RULES2_PATH, "w");
  if (!f) {
    Serial.println("[rules2] saveRules2: failed to open file for write");
    return;
  }

  size_t n = serializeJson(doc, f);
  f.close();

  Serial.printf("[rules2] saveRules2: wrote %u bytes, rules=%u, expr=%u, cond=%u\n",
                (unsigned)n,
                (unsigned)db.rules.size(),
                (unsigned)db.expr.size(),
                (unsigned)db.conditions.size());

  if (doc.overflowed()) {
    Serial.println("[rules2] saveRules2: WARNING doc overflowed (increase capacity)");
  }
}

void loadRules2() {
  if (!LittleFS.exists(RULES2_PATH)) {
    Serial.println("[rules2] loadRules2: no file, starting empty");
    return;
  }

  File f = LittleFS.open(RULES2_PATH, "r");
  if (!f) {
    Serial.println("[rules2] loadRules2: failed to open file");
    return;
  }

  DynamicJsonDocument doc(49152);
  DeserializationError err = deserializeJson(doc, f);
  f.close();

  if (err) {
    Serial.printf("[rules2] loadRules2: JSON parse error: %s\n", err.c_str());
    return;
  }

  uint16_t schema = (uint16_t)(doc["schema"] | 0);
  if (schema != RULES2_SCHEMA) {
    Serial.printf("[rules2] loadRules2: schema mismatch %u (expected %u)\n",
                  (unsigned)schema, (unsigned)RULES2_SCHEMA);
    return;
  }

  // Clear DB first
  db.conditions.clear();
  db.expr.clear();
  db.rules.clear();

  db.nextId = (uint32_t)(doc["nextId"] | 1);

  // ---------------------------------------------------------------------------
  // Conditions
  // ---------------------------------------------------------------------------
  JsonArray conds = doc["conditions"].as<JsonArray>();
  if (!conds.isNull()) {
    for (JsonObject o : conds) {
      Condition c;
      c.id = (uint32_t)(o["id"] | 0);
      c.enabled = (bool)(o["enabled"] | true);
      c.name = String((const char*)(o["name"] | ""));

      c.type = (CondType)(uint8_t)(o["type"] | (uint8_t)CondType::CompareInputToConst);
      c.inputKey = String((const char*)(o["inputKey"] | ""));
      c.op = (CmpOp)(uint8_t)(o["op"] | (uint8_t)CmpOp::GT);

      c.threshold = (float)(o["threshold"] | 0.0);
      c.rhsInputKey = String((const char*)(o["rhsInputKey"] | ""));

      c.stableForMs = (uint32_t)(o["stableForMs"] | 0);

      // Reset runtime
      c.lastEval = false;
      c.lastFlipMs = 0;

      db.conditions.push_back(c);
    }
  }

  // ---------------------------------------------------------------------------
  // Expr nodes
  // ---------------------------------------------------------------------------
  JsonArray expr = doc["expr"].as<JsonArray>();
  if (!expr.isNull()) {
    for (JsonObject o : expr) {
      ExprNode e;
      e.id = (uint32_t)(o["id"] | 0);
      e.type = (ExprType)(uint8_t)(o["type"] | (uint8_t)ExprType::LeafCond);
      e.name = String((const char*)(o["name"] | ""));

      e.condId = (uint32_t)(o["condId"] | 0);
      e.child = (uint32_t)(o["child"] | 0);

      e.children.clear();
      JsonArray kids = o["children"].as<JsonArray>();
      if (!kids.isNull()) {
        for (JsonVariant v : kids) e.children.push_back((uint32_t)(v | 0));
      }

      db.expr.push_back(e);
    }
  }

  // ---------------------------------------------------------------------------
  // Rules + actions
  // ---------------------------------------------------------------------------
  JsonArray rules = doc["rules"].as<JsonArray>();
  if (!rules.isNull()) {
    for (JsonObject o : rules) {
      Rule r;
      r.id = (uint32_t)(o["id"] | 0);
      r.priority = (int16_t)(o["priority"] | 0);
      r.enabled = (bool)(o["enabled"] | true);
      r.name = String((const char*)(o["name"] | ""));

      r.exprRootId = (uint32_t)(o["exprRootId"] | 0);
      r.minEvalPeriodMs = (uint32_t)(o["minEvalPeriodMs"] | 250);
      r.cooldownMs = (uint32_t)(o["cooldownMs"] | 0);

      r.actions.clear();
      JsonArray acts = o["actions"].as<JsonArray>();
      if (!acts.isNull()) {
        for (JsonObject ao : acts) {
          Action a;
          a.type = (ActionType)(uint8_t)(ao["type"] | (uint8_t)ActionType::SetOutput);
          a.outputKey = String((const char*)(ao["outputKey"] | ""));
          a.on = (bool)(ao["on"] | true);
          a.durationMs = (uint32_t)(ao["durationMs"] | 0);
          r.actions.push_back(a);
        }
      }

      // Reset runtime
      r.lastEvalMs = 0;
      r.lastTriggerMs = 0;
      r.lastResult = false;

      db.rules.push_back(r);
    }
  }

  // ---------------------------------------------------------------------------
  // Recompute nextId safely (prevents duplicate IDs)
  // ---------------------------------------------------------------------------
  uint32_t maxId = 0;
  for (const auto& c : db.conditions) if (c.id > maxId) maxId = c.id;
  for (const auto& e : db.expr)       if (e.id > maxId) maxId = e.id;
  for (const auto& r : db.rules)      if (r.id > maxId) maxId = r.id;
  if (db.nextId <= maxId) db.nextId = maxId + 1;

  // ---------------------------------------------------------------------------
  // Optional sanity warnings
  // ---------------------------------------------------------------------------
  for (const auto& r : db.rules) {
    if (r.exprRootId && !db.findExpr(r.exprRootId)) {
      Serial.printf("[rules2] WARN: rule %u root expr %u missing\n",
                    (unsigned)r.id, (unsigned)r.exprRootId);
    }
  }
  for (const auto& e : db.expr) {
    if (e.type == ExprType::LeafCond && e.condId && !db.findCond(e.condId)) {
      Serial.printf("[rules2] WARN: expr %u leaf cond %u missing\n",
                    (unsigned)e.id, (unsigned)e.condId);
    }
  }

  Serial.printf("[rules2] loadRules2: rules=%u, expr=%u, cond=%u, nextId=%u\n",
                (unsigned)db.rules.size(),
                (unsigned)db.expr.size(),
                (unsigned)db.conditions.size(),
                (unsigned)db.nextId);

  if (doc.overflowed()) {
    Serial.println("[rules2] loadRules2: WARNING doc overflowed (increase capacity)");
  }
}



// -----------------------------------------------------------------------------
// Demo bootstrap
// -----------------------------------------------------------------------------
void initRules2Defaults() {
  db = Db{};
  db.nextId = 1;

  // Make two conditions
  uint32_t c1 = uiCreateDefaultCondition();
  if (auto* c = db.findCond(c1)) {
    c->name = "PV V > 50";
    c->inputKey = "pv_v";
    c->type = CondType::CompareInputToConst;
    c->threshold = 50.0f;
    c->stableForMs = 1000;
  }

  uint32_t c2 = uiCreateDefaultCondition();
  if (auto* c = db.findCond(c2)) {
    c->name = "Tank temp > 40";
    c->inputKey = "tank_temp_c";
    c->type = CondType::CompareInputToConst;
    c->threshold = 40.0f;
    c->stableForMs = 0;
  }

  // Create groups to show nesting:
  // g_or = (c1 OR c2)
  uint32_t g_or = uiCreateGroup("Demo OR", true);
  if (auto* g = db.findExpr(g_or)) {
    g->children.push_back(createLeafForCond(c1));
    g->children.push_back(createLeafForCond(c2));
  }

  // Create a rule using g_or root
  Rule r;
  r.id = db.allocId();
  r.enabled = true;
  r.name = "Demo: (PV OR Tank) -> aux1";
  r.exprRootId = g_or;
  r.minEvalPeriodMs = 500;
  r.cooldownMs = 5000;

  Action a;
  a.type = ActionType::SetOutput;
  a.outputKey = "m_aux1";
  a.on = true;
  a.durationMs = 2000;
  r.actions.push_back(a);

  db.rules.push_back(r);
}

} // namespace rules2
