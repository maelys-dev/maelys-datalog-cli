/* SPDX-License-Identifier: MPL-2.0
 * Test-only declaration of the announced additive API. This is not a 0.27 SDK.
 * Injected only when compiling the isolated renderer compatibility binary. */
#ifndef CLI_TEST_SDK_RULE_VARIABLE_NAMES_H
#define CLI_TEST_SDK_RULE_VARIABLE_NAMES_H
#include <maelys/datalog_program.h>
#ifndef MAELYS_DATALOG_HAS_RULE_VARIABLE_NAMES
#define MAELYS_DATALOG_HAS_RULE_VARIABLE_NAMES 1
maelys_datalog_status_t maelys_datalog_program_rule_variable_name(
    const maelys_datalog_program_t *program, size_t rule_index,
    unsigned variable, const char **name);
#endif
#endif
