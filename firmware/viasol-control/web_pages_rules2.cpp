#include "web_pages_rules2.h"
#include "web_pages_rules2_groups.h"
#include "rules2.h"
#include "io_catalog.h"
#include <vector>
#include <algorithm>

// Simple helper: <select> from a key list
static String htmlSelectKeys(const char* name, const char* keys[], int n, const String& cur) {
  String h;
  h += "<select name='" + String(name) + "'>";
  for (int i = 0; i < n; i++) {
    String k = keys[i];
    h += "<option value='" + k + "'";
    if (k == cur) h += " selected";
    h += ">" + k + "</option>";
  }
  h += "</select>";
  return h;
}

static String opToStr2(rules2::CmpOp op) {
  switch (op) {
    case rules2::CmpOp::GT: return "GT";
    case rules2::CmpOp::GE: return "GE";
    case rules2::CmpOp::LT: return "LT";
    case rules2::CmpOp::LE: return "LE";
    case rules2::CmpOp::EQ: return "EQ";
    case rules2::CmpOp::NE: return "NE";
  }
  return "GT";
}

static void pageBegin(String& h, const char* title) {
  h += "<!doctype html><html lang='en'><head>";
  h += "<meta charset='utf-8'>";
  h += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  h += "<title>";
  h += title;
  h += "</title>";
  h += "</head><body>";
}

static void pageEnd(String& h) {
  h += "</body></html>";
}



String buildRules2HomeHtml(Settings& cfg) {
  (void)cfg;
  String h;

  h.reserve(9000);

  h += "<h2>Rules v2 (Parallel)</h2>";
  h += "<p>Separate from Rules v1. Currently in-memory only (persistence stubs).</p>";

  h += "<p>";
  h += "<form method='POST' action='/config/rules2/new' style='display:inline; margin-right:10px;'>";
  h += "<button type='submit'>New Rule</button>";
  h += "</form>";

  h += "<form method='POST' action='/config/rules2/demo' style='display:inline; margin-right:10px;'>";
  h += "<button type='submit'>Load Demo Rules</button>";
  h += "</form>";

  h += "<a href='/config/rules2/conditions' style='margin-right:10px;'>Edit Conditions</a>";
  h += "<a href='/config/rules2/groups'>Edit Groups</a>";
  h += "</p>";

  h += "<h3>Rules</h3>";
  h += "<table border='1' cellpadding='6' cellspacing='0'>";
  h += "<tr><th>ID</th><th>Name</th><th>Enabled</th><th>Pri</th><th>Expr</th><th>minEval</th><th>cooldown</th></tr>";

    // Build sorted render order (do NOT reorder storage)
    std::vector<size_t> order;
    order.reserve(rules2::db.rules.size());

    for (size_t i = 0; i < rules2::db.rules.size(); ++i) {
      order.push_back(i);
    }

    std::sort(order.begin(), order.end(),
      [](size_t a, size_t b) {
        const auto& ra = rules2::db.rules[a];
        const auto& rb = rules2::db.rules[b];

        if (ra.priority != rb.priority)
          return ra.priority < rb.priority;  // higher priority first

        return ra.id < rb.id; // stable tie-breaker
      });

    // Render rows in priority order
    for (size_t idx : order) {
      const auto& r = rules2::db.rules[idx];

      h += "<tr>";
      h += "<td>" + String(r.id) + "</td>";
      h += "<td><a href='/config/rules2/edit?id=" + String(r.id) + "'>" + r.name + "</a></td>";
      h += "<td>" + String(r.enabled ? "yes" : "no") + "</td>";
      h += "<td>" + String(r.priority) + "</td>";
      h += "<td>" + rules2::describeExpr(r.exprRootId) + "</td>";
      h += "<td>" + String(r.minEvalPeriodMs) + "ms</td>";
      h += "<td>" + String(r.cooldownMs) + "ms</td>";
      h += "</tr>";
    }


    h += "</table>";
    h += "<p><a href='/config'>Back</a></p>";
    return h;
  }

String buildRules2ConditionsHtml(Settings& cfg) {
  (void)cfg;
  String h;

  h.reserve(9000);
  pageBegin(h, "Rules v2 Conditions");

  h += "<h2>Rules v2 Conditions</h2>";

  h += "<p>";
  h += "<form method='POST' action='/config/rules2/conditions/new' style='display:inline; margin-right:10px;'>";
  h += "<button type='submit'>New Condition</button>";
  h += "</form>";
  h += "<a href='/config/rules2' style='margin-right:10px;'>Back</a>";
  h += "<a href='/config/rules2/groups'>Edit Groups</a>";
  h += "</p>";

  h += "<form method='POST' action='/config/rules2/conditions/save'>";
  h += "<table border='1' cellpadding='6' cellspacing='0'>";
  h += "<tr>"
       "<th>ID</th><th>En</th><th>Name</th>"
       "<th>LHS Input</th><th>Op</th>"
       "<th>RHS</th><th>Const</th><th>RHS Input</th>"
       "<th>stable(ms)</th><th>Delete</th>"
       "</tr>";

  for (size_t i = 0; i < rules2::db.conditions.size(); i++) {
    auto const& c = rules2::db.conditions[i];
    String base = "c" + String(c.id) + "_";

    h += "<tr>";
    h += "<td>" + String(c.id) + "</td>";

    h += "<td><input type='checkbox' name='" + base + "en'";
    if (c.enabled) h += " checked";
    h += "></td>";

    h += "<td><input name='" + base + "name' value='" + c.name + "'></td>";

    h += "<td>" + htmlSelectKeys((base + "in").c_str(), INPUT_KEYS, N_INPUTS, c.inputKey) + "</td>";

    h += "<td><select name='" + base + "op'>";
    const char* ops[] = {"GT","GE","LT","LE","EQ","NE"};
    for (auto s : ops) {
      h += "<option value='" + String(s) + "'";
      if (String(s) == opToStr2(c.op)) h += " selected";
      h += ">" + String(s) + "</option>";
    }
    h += "</select></td>";

    h += "<td><select name='" + base + "rhs'>";
    h += String("<option value='CONST'") + (c.type == rules2::CondType::CompareInputToConst ? " selected" : "") + ">CONST</option>";
    h += String("<option value='INPUT'") + (c.type == rules2::CondType::CompareInputToInput ? " selected" : "") + ">INPUT</option>";
    h += "</select></td>";

    h += "<td><input name='" + base + "th' value='" + String(c.threshold) + "'></td>";
    h += "<td>" + htmlSelectKeys((base + "rin").c_str(), INPUT_KEYS, N_INPUTS, c.rhsInputKey) + "</td>";
    h += "<td><input name='" + base + "st' value='" + String(c.stableForMs) + "'></td>";

    h += "<td>"
         "<form method='POST' action='/config/rules2/conditions/delete'>"
         "<input type='hidden' name='id' value='" + String(c.id) + "'>"
         "<button type='submit'>X</button>"
         "</form>"
         "</td>";

    h += "</tr>";
  }

  h += "</table>";
  h += "<button type='submit'>Save Conditions</button>";
  h += "</form>";
  
  pageEnd(h);
  return h;
}

String buildRules2EditRuleHtml(Settings& cfg, uint32_t ruleId) {
  (void)cfg;
  auto* r = rules2::db.findRule(ruleId);
  if (!r) return "<p>Rule not found</p><p><a href='/config/rules2'>Back</a></p>";

  String h;
  h.reserve(9000);
  pageBegin(h, "Rules v2 Edit Rules");


  h += "<h2>Edit Rule v2</h2>";

  h += "<form method='POST' action='/config/rules2/save'>";
  h += "<input type='hidden' name='id' value='" + String(r->id) + "'>";

  h += "<p>Name: <input name='name' value='" + r->name + "'></p>";

  h += "<p><label><input type='checkbox' name='en'";
  if (r->enabled) h += " checked";
  h += "> enabled</label></p>";

  h += "<p><label>Priority</label><br>";
  h += "<input type='number' name='priority' value='" + String(r->priority) + "' step='1'></p>";


  h += "<p>minEvalPeriodMs: <input name='minEval' value='" + String(r->minEvalPeriodMs) + "'></p>";
  h += "<p>cooldownMs: <input name='cooldown' value='" + String(r->cooldownMs) + "'></p>";

  // Root group selection (preferred)
  h += "<h3>Expression (Groups)</h3>";
  h += "<p>Root expression:</p>";
  h += "<select name='rootExpr'>";
  h += "<option value='0'";
  if (r->exprRootId == 0) h += " selected";
  h += ">none (auto-disable on save)</option>";

  // List only groups (And/Or) + also allow leaf roots
  for (auto const& e : rules2::db.expr) {
    if (e.type == rules2::ExprType::And || e.type == rules2::ExprType::Or) {
      h += "<option value='" + String(e.id) + "'";
      if (e.id == r->exprRootId) h += " selected";
      h += ">" + rules2::describeExpr(e.id) + "</option>";
    }
  }
  h += "</select>";
  h += " <a href='/config/rules2/groups'>(edit groups)</a>";

  // Keep MVP checkbox builder as a fallback (still useful)
  h += "<details style='margin-top:12px;'>";
  h += "<summary>Advanced: Build a flat AND/OR from conditions (MVP)</summary>";
  h += "<p><i>If you select any conditions here, saving will rebuild a flat group and override the root group selection above.</i></p>";

  String exprModeCur = rules2::getRuleExprModeStr(ruleId);
  h += "<p>Combine selected conditions with:</p>";
  h += "<select name='exprMode'>";
  h += String("<option value='AND'") + (exprModeCur == "AND" ? " selected" : "") + ">AND</option>";
  h += String("<option value='OR'")  + (exprModeCur == "OR"  ? " selected" : "") + ">OR</option>";
  h += "</select>";

  h += "<p>Conditions:</p>";
  if (rules2::db.conditions.empty()) {
    h += "<p><i>No conditions exist yet.</i> <a href='/config/rules2/conditions'>Create one</a></p>";
  } else {
    // Not pre-checking these anymore because this is now a “builder” not the saved root
    for (auto const& c : rules2::db.conditions) {
      String nm = c.name.length() ? c.name : String("cond ") + c.id;
      h += "<label><input type='checkbox' name='cond' value='" + String(c.id) + "'> "
           + nm + " (" + c.inputKey + ")</label><br>";
    }
  }
  h += "</details>";

  h += "<h3>Action</h3>";
  if (r->actions.empty()) {
    h += "<p>Output: " + htmlSelectKeys("out", OUTPUT_KEYS, N_OUTPUTS, "m_relay1") + "</p>";
    h += "<p>On: <select name='on'><option value='1'>1</option><option value='0'>0</option></select></p>";
    h += "<p>Duration ms (0=none): <input name='dur' value='0'></p>";
  } else {
    auto const& a = r->actions[0];
    h += "<p>Output: " + htmlSelectKeys("out", OUTPUT_KEYS, N_OUTPUTS, a.outputKey) + "</p>";
    h += "<p>On: <select name='on'>";
    h += String("<option value='1'") + (a.on ? " selected" : "") + ">1</option>";
    h += String("<option value='0'") + (!a.on ? " selected" : "") + ">0</option>";
    h += "</select></p>";
    h += "<p>Duration ms (0=none): <input name='dur' value='" + String(a.durationMs) + "'></p>";
  }

  h += "<button type='submit'>Save</button>";
  h += "</form>";

  h += "<form method='POST' action='/config/rules2/delete' style='margin-top:12px;'>";
  h += "<input type='hidden' name='id' value='" + String(r->id) + "'>";
  h += "<button type='submit'>Delete Rule</button>";
  h += "</form>";

  h += "<p><a href='/config/rules2'>Back</a></p>";
  pageEnd(h);
  return h;
}
