#include "web_pages_rules2_groups.h"
#include "rules2.h"
#include "io_catalog.h"

static String htmlSelectCondIds(const char* name, uint32_t curId) {
  String h;
  h += "<select name='" + String(name) + "'>";
  h += "<option value='0'>-- select condition --</option>";
  for (auto const& c : rules2::db.conditions) {
    String nm = c.name.length() ? c.name : String("cond ") + c.id;
    h += "<option value='" + String(c.id) + "'";
    if (c.id == curId) h += " selected";
    h += ">" + nm + "</option>";
  }
  h += "</select>";
  return h;
}

static String htmlSelectGroupIds(const char* name, uint32_t excludeId, uint32_t curId) {
  String h;
  h += "<select name='" + String(name) + "'>";
  h += "<option value='0'>-- select group --</option>";
  for (auto const& e : rules2::db.expr) {
    if (!(e.type == rules2::ExprType::And || e.type == rules2::ExprType::Or)) continue;
    if (e.id == excludeId) continue;
    h += "<option value='" + String(e.id) + "'";
    if (e.id == curId) h += " selected";
    h += ">" + rules2::describeExpr(e.id) + "</option>";
  }
  h += "</select>";
  return h;
}

String buildRules2GroupsHtml(Settings& cfg) {
  (void)cfg;
  String h;

  h += "<h2>Rules v2 Groups</h2>";
  h += "<p><a href='/config/rules2' style='margin-right:10px;'>Back</a>";
  h += "<a href='/config/rules2/conditions'>Edit Conditions</a></p>";

  h += "<h3>Create Group</h3>";
  h += "<form method='POST' action='/config/rules2/groups/new'>";
  h += "<p>Name: <input name='name' value='New group'></p>";
  h += "<p>Type: <select name='gtype'><option value='AND'>AND</option><option value='OR'>OR</option></select></p>";
  h += "<button type='submit'>Create</button>";
  h += "</form>";

  h += "<h3>Existing Groups</h3>";
  h += "<table border='1' cellpadding='6' cellspacing='0'>";
  h += "<tr><th>ID</th><th>Name</th><th>Type</th><th>Children</th><th>Edit</th><th>Delete</th></tr>";

  for (auto const& e : rules2::db.expr) {
    if (!(e.type == rules2::ExprType::And || e.type == rules2::ExprType::Or)) continue;
    String t = (e.type == rules2::ExprType::Or) ? "OR" : "AND";
    h += "<tr>";
    h += "<td>" + String(e.id) + "</td>";
    h += "<td>" + (e.name.length() ? e.name : String("group ") + e.id) + "</td>";
    h += "<td>" + t + "</td>";
    h += "<td>" + String((int)e.children.size()) + "</td>";
    h += "<td><a href='/config/rules2/group?id=" + String(e.id) + "'>edit</a></td>";
    h += "<td><form method='POST' action='/config/rules2/groups/delete'>"
         "<input type='hidden' name='id' value='" + String(e.id) + "'>"
         "<button type='submit'>X</button></form></td>";
    h += "</tr>";
  }

  h += "</table>";
  return h;
}

String buildRules2EditGroupHtml(Settings& cfg, uint32_t groupId) {
  (void)cfg;
  auto* g = rules2::db.findExpr(groupId);
  if (!g || !(g->type == rules2::ExprType::And || g->type == rules2::ExprType::Or)) {
    return "<p>Group not found</p><p><a href='/config/rules2/groups'>Back</a></p>";
  }

  String h;
  h += "<h2>Edit Group</h2>";
  h += "<p><a href='/config/rules2/groups'>Back</a></p>";

  // Save group meta
  h += "<form method='POST' action='/config/rules2/group/save'>";
  h += "<input type='hidden' name='id' value='" + String(g->id) + "'>";
  h += "<p>Name: <input name='name' value='" + g->name + "'></p>";
  h += "<p>Type: <select name='gtype'>";
  h += String("<option value='AND'") + (g->type == rules2::ExprType::And ? " selected" : "") + ">AND</option>";
  h += String("<option value='OR'")  + (g->type == rules2::ExprType::Or  ? " selected" : "") + ">OR</option>";
  h += "</select></p>";
  h += "<button type='submit'>Save Group</button>";
  h += "</form>";

  // Children list
  h += "<h3>Children</h3>";
  if (g->children.empty()) {
    h += "<p><i>No children yet.</i></p>";
  } else {
    h += "<table border='1' cellpadding='6' cellspacing='0'>";
    h += "<tr><th>#</th><th>Expr</th><th>Remove</th></tr>";
    for (int i = 0; i < (int)g->children.size(); i++) {
      uint32_t cid = g->children[i];
      h += "<tr>";
      h += "<td>" + String(i) + "</td>";
      h += "<td>" + rules2::describeExpr(cid) + "</td>";
      h += "<td><form method='POST' action='/config/rules2/group/removeChild'>"
           "<input type='hidden' name='gid' value='" + String(g->id) + "'>"
           "<input type='hidden' name='idx' value='" + String(i) + "'>"
           "<button type='submit'>remove</button></form></td>";
      h += "</tr>";
    }
    h += "</table>";
  }

  // Add condition -> leaf
  h += "<h3>Add Condition</h3>";
  h += "<form method='POST' action='/config/rules2/group/addChild'>";
  h += "<input type='hidden' name='gid' value='" + String(g->id) + "'>";
  h += htmlSelectCondIds("addCond", 0);
  h += " <button type='submit'>Add</button>";
  h += "</form>";

  // Add group -> nesting
  h += "<h3>Add Group (nest)</h3>";
  h += "<form method='POST' action='/config/rules2/group/addChild'>";
  h += "<input type='hidden' name='gid' value='" + String(g->id) + "'>";
  h += htmlSelectGroupIds("addGroup", g->id, 0);
  h += " <button type='submit'>Add</button>";
  h += "</form>";

  return h;
}
