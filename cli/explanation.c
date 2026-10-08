/* SPDX-License-Identifier: MPL-2.0 */
#include "explanation.h"
static const char *name(const char *const *names, size_t count, unsigned code) {
    return code < count ? names[code] : "unknown";
}
static const char *origin(unsigned code) {
    static const char *const names[] = {"not-applicable", "policy-fact", "edb", "idb", "not-applicable"};
    return name(names, sizeof names / sizeof names[0], code);
}
static void value(maelys_cli_json_writer_t *j, const maelys_datalog_value_t *v) {
    (void)maelys_cli_json_begin_object(j);
    if (v->kind == MAELYS_DATALOG_VALUE_SYMBOL) {
        (void)maelys_cli_json_key_string(j, "kind", "symbol");
        (void)maelys_cli_json_key_string(j, "value", v->as.symbol);
    } else if (v->kind == MAELYS_DATALOG_VALUE_INTEGER) {
        (void)maelys_cli_json_key_string(j, "kind", "integer");
        (void)maelys_cli_json_key_integer(j, "value", v->as.integer);
    } else if (v->kind == MAELYS_DATALOG_VALUE_BOOLEAN) {
        (void)maelys_cli_json_key_string(j, "kind", "boolean");
        (void)maelys_cli_json_key_boolean(j, "value", v->as.boolean);
    }
    (void)maelys_cli_json_end_object(j);
}
static void fact(maelys_cli_json_writer_t *j, const maelys_datalog_fact_t *f) {
    (void)maelys_cli_json_begin_object(j);
    (void)maelys_cli_json_key_string(j, "predicate", f->predicate);
    (void)maelys_cli_json_key(j, "terms"); (void)maelys_cli_json_begin_array(j);
    for (size_t i = 0; i < f->arity; ++i) value(j, &f->terms[i]);
    (void)maelys_cli_json_end_array(j); (void)maelys_cli_json_end_object(j);
}
static void atom(maelys_cli_json_writer_t *j, const maelys_datalog_ir_atom_t *a) {
    (void)maelys_cli_json_begin_object(j);
    (void)maelys_cli_json_key_string(j, "predicate", a->predicate);
    (void)maelys_cli_json_key(j, "terms"); (void)maelys_cli_json_begin_array(j);
    for (size_t i = 0; i < a->arity; ++i) {
        const maelys_datalog_ir_term_t *t = &a->terms[i];
        if (t->kind == MAELYS_DATALOG_IR_VARIABLE) {
            (void)maelys_cli_json_begin_object(j);
            (void)maelys_cli_json_key_string(j, "kind", "variable");
            (void)maelys_cli_json_key_unsigned(j, "value", t->as.variable);
            (void)maelys_cli_json_end_object(j);
        } else {
            maelys_datalog_value_t v = {0};
            if (t->kind == MAELYS_DATALOG_IR_SYMBOL) {v.kind = MAELYS_DATALOG_VALUE_SYMBOL; v.as.symbol = t->as.symbol;}
            else if (t->kind == MAELYS_DATALOG_IR_INTEGER) {v.kind = MAELYS_DATALOG_VALUE_INTEGER; v.as.integer = t->as.integer;}
            else if (t->kind == MAELYS_DATALOG_IR_BOOLEAN) {v.kind = MAELYS_DATALOG_VALUE_BOOLEAN; v.as.boolean = t->as.boolean;}
            value(j, &v);
        }
    }
    (void)maelys_cli_json_end_array(j); (void)maelys_cli_json_end_object(j);
}
static void comparison(maelys_cli_json_writer_t *j, maelys_datalog_ir_comparison_t op,
                       const maelys_datalog_value_t *lhs, const maelys_datalog_value_t *rhs) {
    static const char *const names[] = {"unknown", "equal", "not-equal", "less", "less-or-equal", "greater", "greater-or-equal"};
    (void)maelys_cli_json_key(j, "comparison"); (void)maelys_cli_json_begin_object(j);
    (void)maelys_cli_json_key_string(j, "operator", name(names, sizeof names / sizeof names[0], (unsigned)op));
    (void)maelys_cli_json_key(j, "lhs"); value(j, lhs);
    (void)maelys_cli_json_key(j, "rhs"); value(j, rhs);
    (void)maelys_cli_json_end_object(j);
}
static void filter(maelys_cli_json_writer_t *j, size_t index, const maelys_datalog_value_t *v) {
    (void)maelys_cli_json_key(j, "filter"); (void)maelys_cli_json_begin_object(j);
    (void)maelys_cli_json_key_unsigned(j, "programIndex", index);
    (void)maelys_cli_json_key(j, "value"); value(j, v);
    (void)maelys_cli_json_end_object(j);
}
maelys_datalog_status_t datalog_cli_explanation_json(maelys_cli_json_writer_t *j,
        const maelys_datalog_prepared_explanation_t *prepared, const maelys_datalog_fact_t *query) {
    maelys_datalog_explanation_info_t info = {0};
    maelys_datalog_status_t rc = maelys_datalog_prepared_explanation_info(prepared, &info);
    if (rc != MAELYS_DATALOG_STATUS_OK) return rc;
    (void)maelys_cli_json_begin_object(j);
    (void)maelys_cli_json_key_string(j, "kind", info.kind == MAELYS_DATALOG_EXPLAIN_TRUE ? "why-true" : "why-false");
    (void)maelys_cli_json_key_boolean(j, "found", info.found);
    (void)maelys_cli_json_key_boolean(j, "truncated", info.truncated);
    static const char *const statuses[] = {"unknown", "not-applicable", "complete", "truncated"};
    (void)maelys_cli_json_key_string(j, "status", info.kind == MAELYS_DATALOG_EXPLAIN_TRUE
        ? (info.truncated ? "truncated" : info.found ? "complete" : "not-derived")
        : name(statuses, sizeof statuses / sizeof statuses[0], info.false_status));
    if (info.kind == MAELYS_DATALOG_EXPLAIN_FALSE) {
    (void)maelys_cli_json_key_string(j, "summary", info.false_summary == MAELYS_DATALOG_WHY_FALSE_SUMMARY_NO_CANDIDATE_RULE ? "no-candidate-rule" : "none");
    (void)maelys_cli_json_key(j, "limits"); (void)maelys_cli_json_begin_array(j);
    static const char *const limits[] = {"candidate-rules", "substitutions", "depth", "diagnostics", "filter-cost"};
    for (size_t i = 0; i < sizeof limits / sizeof limits[0]; ++i) {
        if (info.limit_hits & (1u << i)) (void)maelys_cli_json_string(j, limits[i]);
    }
    (void)maelys_cli_json_end_array(j);
    }
    (void)maelys_cli_json_key(j, "query"); fact(j, query);
    if (info.kind == MAELYS_DATALOG_EXPLAIN_FALSE) {
    (void)maelys_cli_json_key_string(j, "queryOrigin", origin(info.query_origin));
    (void)maelys_cli_json_key_unsigned(j, "candidateRuleCount", info.candidate_rule_count);
    (void)maelys_cli_json_key_unsigned(j, "substitutionCount", info.substitution_count);
    (void)maelys_cli_json_key_unsigned(j, "filterCostUnits", info.filter_cost_units);
    (void)maelys_cli_json_key_unsigned(j, "limitHits", info.limit_hits);
    }
    (void)maelys_cli_json_key(j, "steps"); (void)maelys_cli_json_begin_array(j);
    for (size_t i = 0; i < info.step_count; ++i) {
        maelys_datalog_explanation_step_view_t s = {0};
        rc = maelys_datalog_prepared_explanation_step(prepared, i, &s);
        if (rc != MAELYS_DATALOG_STATUS_OK) return rc;
        (void)maelys_cli_json_begin_object(j);
        (void)maelys_cli_json_key_unsigned(j, "index", i);
        (void)maelys_cli_json_key_unsigned(j, "ruleId", s.rule_id);
        (void)maelys_cli_json_key(j, "fact"); fact(j, &s.fact);
        (void)maelys_cli_json_key(j, "premises"); (void)maelys_cli_json_begin_array(j);
        for (size_t n = 0; n < s.premise_count; ++n) {
            maelys_datalog_explanation_premise_view_t p = {0};
            rc = maelys_datalog_prepared_explanation_premise(prepared, s.premise_begin + n, &p);
            if (rc != MAELYS_DATALOG_STATUS_OK) return rc;
            static const char *const names[] = {"unknown", "positive-fact", "negated-absence", "comparison-true", "filter-true", "count", "min", "max", "sum"};
            (void)maelys_cli_json_begin_object(j);
            (void)maelys_cli_json_key_string(j, "kind", name(names, sizeof names / sizeof names[0], p.kind));
            (void)maelys_cli_json_key_string(j, "origin", origin(p.origin));
            (void)maelys_cli_json_key_unsigned(j, "bodyIndex", p.body_index);
            (void)maelys_cli_json_key(j, "parentStep");
            if (p.parent_step == MAELYS_DATALOG_EXPLANATION_NO_STEP) (void)maelys_cli_json_null(j);
            else (void)maelys_cli_json_unsigned(j, p.parent_step);
            if (p.atom.predicate) {(void)maelys_cli_json_key(j, "atom"); atom(j, &p.atom);}
            if (p.kind == MAELYS_DATALOG_EXPLANATION_PREMISE_COMPARISON_TRUE) comparison(j, p.op, &p.lhs, &p.rhs);
            if (p.kind == MAELYS_DATALOG_EXPLANATION_PREMISE_FILTER_TRUE) filter(j, p.filter_program_index, &p.filter_value);
            if (p.kind >= MAELYS_DATALOG_EXPLANATION_PREMISE_COUNT && p.kind <= MAELYS_DATALOG_EXPLANATION_PREMISE_SUM) {
                (void)maelys_cli_json_key_unsigned(j, "projectedVariable", p.projected_variable);
                (void)maelys_cli_json_key_unsigned(j, "aggregateValue", p.aggregate_value);
            }
            (void)maelys_cli_json_end_object(j);
        }
        (void)maelys_cli_json_end_array(j); (void)maelys_cli_json_end_object(j);
    }
    (void)maelys_cli_json_end_array(j);
    (void)maelys_cli_json_key(j, "obstacles"); (void)maelys_cli_json_begin_array(j);
    for (size_t i = 0; i < info.diagnostic_count; ++i) {
        maelys_datalog_explanation_obstacle_view_t o = {0};
        rc = maelys_datalog_prepared_explanation_obstacle(prepared, i, &o);
        if (rc != MAELYS_DATALOG_STATUS_OK) return rc;
        static const char *const names[] = {"unknown", "positive-no-match", "negative-contradicted", "comparison-false", "recursive-no-base-support", "filter-false", "count-mismatch", "min-mismatch", "max-mismatch", "sum-mismatch", "min-empty", "max-empty"};
        (void)maelys_cli_json_begin_object(j);
        (void)maelys_cli_json_key_unsigned(j, "ruleId", o.rule_id);
        (void)maelys_cli_json_key_unsigned(j, "depth", o.depth);
        (void)maelys_cli_json_key(j, "target"); fact(j, &o.target);
        (void)maelys_cli_json_key_string(j, "kind", name(names, sizeof names / sizeof names[0], o.obstacle_kind));
        (void)maelys_cli_json_key_string(j, "origin", origin(o.obstacle_origin));
        (void)maelys_cli_json_key_unsigned(j, "bodyIndex", o.body_index);
        (void)maelys_cli_json_key_unsigned(j, "unboundTermMask", o.unbound_term_mask);
        if (o.pattern.predicate) {(void)maelys_cli_json_key(j, "pattern"); atom(j, &o.pattern);}
        if (o.obstacle_kind == MAELYS_DATALOG_WHY_FALSE_OBSTACLE_COMPARISON_FALSE) comparison(j, o.op, &o.lhs, &o.rhs);
        if (o.obstacle_kind == MAELYS_DATALOG_WHY_FALSE_OBSTACLE_FILTER_FALSE) filter(j, o.filter_program_index, &o.filter_value);
        (void)maelys_cli_json_key(j, "bindings"); (void)maelys_cli_json_begin_array(j);
        for (size_t n = 0; n < MAELYS_DATALOG_IR_MAX_VARIABLES; ++n) {
            if (!(o.bound_variable_mask & (UINT32_C(1) << n))) continue;
            (void)maelys_cli_json_begin_object(j);
            (void)maelys_cli_json_key_unsigned(j, "variable", n);
            (void)maelys_cli_json_key(j, "value"); value(j, &o.substitution[n]);
            (void)maelys_cli_json_end_object(j);
        }
        (void)maelys_cli_json_end_array(j);
        (void)maelys_cli_json_key(j, "supports"); (void)maelys_cli_json_begin_array(j);
        for (size_t n = 0; n < o.support_count; ++n) {
            (void)maelys_cli_json_begin_object(j);
            (void)maelys_cli_json_key_unsigned(j, "bodyIndex", o.supports[n].body_index);
            (void)maelys_cli_json_key_string(j, "origin", origin(o.supports[n].origin));
            (void)maelys_cli_json_key(j, "fact"); fact(j, &o.supports[n].fact);
            (void)maelys_cli_json_end_object(j);
        }
        (void)maelys_cli_json_end_array(j); (void)maelys_cli_json_end_object(j);
    }
    (void)maelys_cli_json_end_array(j); (void)maelys_cli_json_end_object(j);
    return MAELYS_DATALOG_STATUS_OK;
}
