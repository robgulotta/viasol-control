#pragma once
#include <Arduino.h>
#include <vector>
#include <WebServer.h>

// Uses your existing IO catalog interface (from io_catalog.h)
float inputValueByKey(const String& key);
void applyOutput(const String& outputKey, bool on);

namespace rules2 {

// -----------------------------------------------------------------------------
// Comparators
// -----------------------------------------------------------------------------
enum class CmpOp : uint8_t { GT, GE, LT, LE, EQ, NE };

// -----------------------------------------------------------------------------
// Condition blocks (atomic IFs)
// -----------------------------------------------------------------------------
enum class CondType : uint8_t {
  CompareInputToConst,   // inputKey op threshold
  CompareInputToInput    // inputKey op rhsInputKey
};

struct Condition {
  uint32_t id = 0;
  bool enabled = true;
  String name;

  CondType type = CondType::CompareInputToConst;

  String inputKey;
  CmpOp op = CmpOp::GT;

  // RHS = CONST
  float threshold = 0.0f;

  // RHS = INPUT
  String rhsInputKey;

  // Stability helpers
  uint32_t stableForMs = 0;

  // Runtime state
  bool lastEval = false;
  uint32_t lastFlipMs = 0;
};

// -----------------------------------------------------------------------------
// Expression tree (AND / OR / NOT + leaf condition references)
// Groups are ExprNodes with type And/Or and a name.
// -----------------------------------------------------------------------------
enum class ExprType : uint8_t { LeafCond, And, Or, Not };

struct ExprNode {
  uint32_t id = 0;
  ExprType type = ExprType::LeafCond;
  String name;                 // used for groups (and can be used for leaves too)

  // LeafCond
  uint32_t condId = 0;

  // Not
  uint32_t child = 0;

  // And / Or
  std::vector<uint32_t> children;
};

// -----------------------------------------------------------------------------
// Actions
// -----------------------------------------------------------------------------
enum class ActionType : uint8_t { SetOutput };

struct Action {
  ActionType type = ActionType::SetOutput;
  String outputKey;
  bool on = true;
  uint32_t durationMs = 0;   // 0 = no hold
};

// -----------------------------------------------------------------------------
// Rule object
// -----------------------------------------------------------------------------
struct Rule {
  uint32_t id = 0;
  int16_t priority = 0;   // higher = wins conflicts, UI sorting
  bool enabled = true;
  String name;

  uint32_t exprRootId = 0;
  std::vector<Action> actions;

  // Timing controls
  uint32_t minEvalPeriodMs = 250; // frequency limit
  uint32_t cooldownMs = 0;        // lockout after trigger

  // Runtime state
  uint32_t lastEvalMs = 0;
  uint32_t lastTriggerMs = 0;
  bool lastResult = false;
};

// -----------------------------------------------------------------------------
// In-memory database
// -----------------------------------------------------------------------------
struct Db {
  std::vector<Condition> conditions;
  std::vector<ExprNode> expr;
  std::vector<Rule> rules;

  uint32_t nextId = 1;

  uint32_t allocId() { return nextId++; }

  Condition* findCond(uint32_t id);
  ExprNode* findExpr(uint32_t id);
  Rule* findRule(uint32_t id);
};

// Global Rules v2 DB
extern Db db;

// -----------------------------------------------------------------------------
// Engine
// -----------------------------------------------------------------------------
void processRules2();
bool evalCondition(Condition& c, uint32_t nowMs);
bool evalExpr(uint32_t exprId, uint32_t nowMs);

// -----------------------------------------------------------------------------
// UI helpers (conditions + rules)
// -----------------------------------------------------------------------------
uint32_t uiCreateDefaultCondition();
void uiDeleteCondition(uint32_t id);
void uiSaveConditionsFromPost(WebServer& s);

uint32_t uiCreateDefaultRule();
void uiDeleteRule(uint32_t id);
void uiSaveRuleFromPost(WebServer& s);

// -----------------------------------------------------------------------------
// Groups (ExprNode) helpers for nested logic
// -----------------------------------------------------------------------------
uint32_t uiCreateGroup(const String& name, bool isOr);  // And/Or group
void uiDeleteExprNode(uint32_t exprId);                // basic delete
void uiSaveGroupFromPost(WebServer& s);                // save name + type
void uiAddChildToGroupFromPost(WebServer& s);          // add condition leaf or group child
void uiRemoveChildFromGroupFromPost(WebServer& s);     // remove by index

// -----------------------------------------------------------------------------
// Introspection helpers (for UI pre-check / display)
// -----------------------------------------------------------------------------
void getRuleSelectedCondIds(uint32_t ruleId, std::vector<uint32_t>& out);
const char* getRuleExprModeStr(uint32_t ruleId); // "AND" or "OR"
String describeExpr(uint32_t exprId);            // short text for UI

// -----------------------------------------------------------------------------
// Persistence (stub now, real Preferences later)
// -----------------------------------------------------------------------------
void loadRules2();
void saveRules2();

// -----------------------------------------------------------------------------
// Bootstrap / demo
// -----------------------------------------------------------------------------
void initRules2Defaults();

} // namespace rules2
