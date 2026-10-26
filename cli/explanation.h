/* SPDX-License-Identifier: MPL-2.0 */
#ifndef DATALOG_CLI_EXPLANATION_H
#define DATALOG_CLI_EXPLANATION_H
#include <maelys/cli.h>
#include <maelys/datalog_explanations.h>
maelys_datalog_status_t datalog_cli_explanation_json(
    maelys_cli_json_writer_t *, const maelys_datalog_prepared_explanation_t *,
    const maelys_datalog_fact_t *query);
#endif
