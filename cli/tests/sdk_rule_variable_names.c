/* SPDX-License-Identifier: MPL-2.0
 * Test double for successful names and failed lookup, over a real 0.26 program.
 * Never installed, and never used by the product executable. */
#include "sdk_rule_variable_names.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

maelys_datalog_status_t maelys_datalog_program_rule_variable_name(
    const maelys_datalog_program_t *program, size_t rule_index,
    unsigned variable, const char **name) {
    maelys_datalog_ir_rule_t rule = {0};
    assert(program && name);
    assert(maelys_datalog_program_rule(program, rule_index, &rule) == MAELYS_DATALOG_STATUS_OK);
    assert(rule_index < 2);
    assert(strcmp(rule.head.predicate, rule_index ? "can_deliver" : "allow") == 0);
    if (getenv("DATALOG_CLI_TEST_NAMES_UNAVAILABLE")) return MAELYS_DATALOG_STATUS_NOT_FOUND;
    const char *which = getenv("DATALOG_CLI_TEST_NAMES_CASE");
    assert(which);
    if (variable >= 26u) return MAELYS_DATALOG_STATUS_NOT_FOUND;
    if (strcmp(which, "mixed") == 0) {
        if (variable == 20u) *name = "U";
        else {assert(rule_index == 0u && variable == 0u); *name = "Resource";}
    } else if (rule_index || strcmp(which, "anonymous") == 0) {
        assert(variable == 0u); *name = "User";
    } else {
        assert(variable < 2u);
        const int reversed = strcmp(which, "reversed") == 0;
        *name = (variable == (unsigned)reversed) ? "User" : "Resource";
    }
    return MAELYS_DATALOG_STATUS_OK;
}
