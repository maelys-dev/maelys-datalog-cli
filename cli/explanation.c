/* SPDX-License-Identifier: MPL-2.0 */
#include "explanation.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

/* Human presentation consumes the public typed views while their lease is live.
 * The SDK document and JSON serializer above remain the machine contract. */
typedef struct {
    char *data;
    size_t length, capacity;
    int failed;
    const maelys_cli_context_t *context;
    /* Rule whose variables are being rendered: its explanation rule_id, or 0. */
    const maelys_datalog_program_t *program;
    size_t rule_id;
} explanation_text_t;

static const char *translated(explanation_text_t *out, const char *source) {
    const maelys_cli_language_t *language = out->context->language;
    if (language) for (size_t i = 0; i < language->translation_count; ++i) {
        const maelys_cli_translation_t *entry = &language->translations[i];
        if (strcmp(entry->source, source) == 0) return entry->text;
    }
    return source;
}
static void append(explanation_text_t *out, const char *s) {
    if (out->failed) return;
    size_t length = strlen(s);
    if (length > SIZE_MAX - out->length - 1u) {out->failed = 1; return;}
    size_t needed = out->length + length + 1u;
    if (needed > out->capacity) {
        size_t capacity = out->capacity ? out->capacity : 256u;
        while (capacity < needed) {
            if (capacity > SIZE_MAX / 2u) {capacity = needed; break;}
            capacity *= 2u;
        }
        char *grown = realloc(out->data, capacity);
        if (!grown) {out->failed = 1; return;}
        out->data = grown; out->capacity = capacity;
    }
    memcpy(out->data + out->length, s, length + 1u); out->length += length;
}
static void sentence(explanation_text_t *out, const char *s) {
    append(out, translated(out, s));
}
static void number(explanation_text_t *out, size_t n) {
    char text[32]; (void)snprintf(text, sizeof text, "%zu", n); append(out, text);
}
static void human_value(explanation_text_t *out, const maelys_datalog_value_t *v) {
    if (v->kind == MAELYS_DATALOG_VALUE_SYMBOL) {
        static const char hex[] = "0123456789abcdef";
        append(out, "\"");
        for (const unsigned char *p = (const unsigned char *)v->as.symbol; *p; ++p) {
            if (*p == '"' || *p == '\\') {
                char s[] = {'\\', (char)*p, 0}; append(out, s);
            } else if (*p < 0x20u) {
                char s[] = {'\\', 'u', '0', '0', hex[*p >> 4], hex[*p & 15], 0}; append(out, s);
            } else {char s[] = {(char)*p, 0}; append(out, s);}
        }
        append(out, "\"");
    } else if (v->kind == MAELYS_DATALOG_VALUE_INTEGER) {
        char text[32]; (void)snprintf(text, sizeof text, "%" PRId64, v->as.integer); append(out, text);
    } else if (v->kind == MAELYS_DATALOG_VALUE_BOOLEAN) append(out, v->as.boolean ? "true" : "false");
    else out->failed = 1;
}
static void human_fact(explanation_text_t *out, const maelys_datalog_fact_t *f) {
    append(out, f->predicate); append(out, "(");
    for (size_t i = 0; i < f->arity; ++i) {if (i) append(out, ", "); human_value(out, &f->terms[i]);}
    append(out, ").");
}
static void variable(explanation_text_t *out, unsigned id) {
    /* The SDK names a variable only when the source spelling was retained; a
     * numeric ID is never decoded into a name. Absent names keep the SDK's ?N. */
    const char *source_name = NULL;
    if (out->program && out->rule_id &&
        maelys_datalog_program_rule_variable_name(out->program, out->rule_id - 1u, id,
                                                  &source_name) == MAELYS_DATALOG_STATUS_OK)
        append(out, source_name);
    else {append(out, "?"); number(out, id);}
}
static void human_atom(explanation_text_t *out, const maelys_datalog_ir_atom_t *a) {
    append(out, a->predicate); append(out, "(");
    for (size_t i = 0; i < a->arity; ++i) {
        if (i) append(out, ", ");
        const maelys_datalog_ir_term_t *t = &a->terms[i];
        if (t->kind == MAELYS_DATALOG_IR_VARIABLE) variable(out, t->as.variable);
        else {
            maelys_datalog_value_t v = {0};
            if (t->kind == MAELYS_DATALOG_IR_SYMBOL) {v.kind = MAELYS_DATALOG_VALUE_SYMBOL; v.as.symbol = t->as.symbol;}
            else if (t->kind == MAELYS_DATALOG_IR_INTEGER) {v.kind = MAELYS_DATALOG_VALUE_INTEGER; v.as.integer = t->as.integer;}
            else {v.kind = MAELYS_DATALOG_VALUE_BOOLEAN; v.as.boolean = t->as.boolean;}
            human_value(out, &v);
        }
    }
    append(out, ").");
}
static void human_comparison(explanation_text_t *out, maelys_datalog_ir_comparison_t op,
    const maelys_datalog_value_t *lhs, const maelys_datalog_value_t *rhs) {
    static const char *const operators[] = {"?", "=", "!=", "<", "<=", ">", ">="};
    human_value(out, lhs); append(out, " ");
    append(out, name(operators, sizeof operators / sizeof operators[0], (unsigned)op));
    append(out, " "); human_value(out, rhs);
}
static void human_origin(explanation_text_t *out, unsigned code) {
    if (code == 1u) sentence(out, " (policy fact)");
    else if (code == 2u) sentence(out, " (input fact)");
    else if (code == 3u) sentence(out, " (derived fact)");
}
static maelys_datalog_status_t human_premise(explanation_text_t *out,
    const maelys_datalog_explanation_premise_view_t *p) {
    append(out, "  - ");
    switch (p->kind) {
    case MAELYS_DATALOG_EXPLANATION_PREMISE_POSITIVE_FACT:
        human_atom(out, &p->atom); human_origin(out, p->origin); break;
    case MAELYS_DATALOG_EXPLANATION_PREMISE_NEGATED_ABSENCE:
        sentence(out, "No matching fact: "); human_atom(out, &p->atom); break;
    case MAELYS_DATALOG_EXPLANATION_PREMISE_COMPARISON_TRUE:
        sentence(out, "The comparison holds: "); human_comparison(out, p->op, &p->lhs, &p->rhs); append(out, "."); break;
    case MAELYS_DATALOG_EXPLANATION_PREMISE_FILTER_TRUE:
        sentence(out, "The filter accepts "); human_value(out, &p->filter_value); append(out, "."); break;
    case MAELYS_DATALOG_EXPLANATION_PREMISE_COUNT:
    case MAELYS_DATALOG_EXPLANATION_PREMISE_MIN:
    case MAELYS_DATALOG_EXPLANATION_PREMISE_MAX:
    case MAELYS_DATALOG_EXPLANATION_PREMISE_SUM: {
        static const char *const labels[] = {"The count condition holds for ", "The minimum condition holds for ", "The maximum condition holds for ", "The sum condition holds for "};
        sentence(out, labels[p->kind - MAELYS_DATALOG_EXPLANATION_PREMISE_COUNT]);
        human_atom(out, &p->atom); break;
    }
    default: return MAELYS_DATALOG_STATUS_UNSUPPORTED;
    }
    append(out, "\n"); return MAELYS_DATALOG_STATUS_OK;
}
static maelys_datalog_status_t human_obstacle(explanation_text_t *out,
    const maelys_datalog_explanation_obstacle_view_t *o) {
    sentence(out, "\nRule "); number(out, o->rule_id); sentence(out, " cannot derive ");
    human_fact(out, &o->target); append(out, "\n");
    out->rule_id = o->rule_id;
    if (o->bound_variable_mask) {
        sentence(out, "  With "); int first = 1;
        for (unsigned n = 0; n < MAELYS_DATALOG_IR_MAX_VARIABLES; ++n) {
            if (!(o->bound_variable_mask & (UINT32_C(1) << n))) continue;
            if (!first) append(out, ", ");
            first = 0;
            variable(out, n); append(out, " = "); human_value(out, &o->substitution[n]);
        }
        append(out, ".\n");
    }
    for (size_t n = 0; n < o->support_count; ++n) {
        sentence(out, "  Already satisfied: "); human_fact(out, &o->supports[n].fact);
        human_origin(out, o->supports[n].origin); append(out, "\n");
    }
    append(out, "  ");
    switch (o->obstacle_kind) {
    case MAELYS_DATALOG_WHY_FALSE_OBSTACLE_POSITIVE_NO_MATCH:
        sentence(out, "No matching fact for "); human_atom(out, &o->pattern); break;
    case MAELYS_DATALOG_WHY_FALSE_OBSTACLE_NEGATIVE_CONTRADICTED:
        sentence(out, "Negation fails because this fact exists: "); human_atom(out, &o->pattern); human_origin(out, o->obstacle_origin); break;
    case MAELYS_DATALOG_WHY_FALSE_OBSTACLE_COMPARISON_FALSE:
        sentence(out, "The comparison fails: "); human_comparison(out, o->op, &o->lhs, &o->rhs); append(out, "."); break;
    case MAELYS_DATALOG_WHY_FALSE_OBSTACLE_RECURSIVE_NO_BASE_SUPPORT:
        sentence(out, "Recursion has no supporting fact to start from: "); human_atom(out, &o->pattern); break;
    case MAELYS_DATALOG_WHY_FALSE_OBSTACLE_FILTER_FALSE:
        sentence(out, "The filter rejects "); human_value(out, &o->filter_value); append(out, "."); break;
    case MAELYS_DATALOG_WHY_FALSE_OBSTACLE_COUNT_MISMATCH:
    case MAELYS_DATALOG_WHY_FALSE_OBSTACLE_MIN_MISMATCH:
    case MAELYS_DATALOG_WHY_FALSE_OBSTACLE_MAX_MISMATCH:
    case MAELYS_DATALOG_WHY_FALSE_OBSTACLE_SUM_MISMATCH: {
        static const char *const labels[] = {"The number of matching facts differs from the requested value: ", "The minimum differs from the requested value: ", "The maximum differs from the requested value: ", "The sum differs from the requested value: "};
        sentence(out, labels[o->obstacle_kind - MAELYS_DATALOG_WHY_FALSE_OBSTACLE_COUNT_MISMATCH]);
        human_atom(out, &o->pattern); break;
    }
    case MAELYS_DATALOG_WHY_FALSE_OBSTACLE_MIN_EMPTY:
    case MAELYS_DATALOG_WHY_FALSE_OBSTACLE_MAX_EMPTY:
        sentence(out, o->obstacle_kind == MAELYS_DATALOG_WHY_FALSE_OBSTACLE_MIN_EMPTY
            ? "The minimum has no matching input fact: " : "The maximum has no matching input fact: ");
        human_atom(out, &o->pattern); break;
    default: return MAELYS_DATALOG_STATUS_UNSUPPORTED;
    }
    append(out, "\n"); return MAELYS_DATALOG_STATUS_OK;
}
maelys_datalog_status_t datalog_cli_explanation_text(const maelys_cli_context_t *context,
    const maelys_datalog_prepared_explanation_t *prepared,
    const maelys_datalog_program_t *program, const maelys_datalog_fact_t *query, char **text) {
    explanation_text_t out = {.context = context, .program = program};
    maelys_datalog_explanation_info_t info = {0};
    maelys_datalog_status_t rc = maelys_datalog_prepared_explanation_info(prepared, &info);
    if (rc != MAELYS_DATALOG_STATUS_OK) return rc;
    if (info.kind == MAELYS_DATALOG_EXPLAIN_TRUE) {
        sentence(&out, info.found ? "The fact is present: " : "The fact is not derived: ");
        human_fact(&out, query); append(&out, "\n");
        if (!info.found) sentence(&out, "No derivation is available. Use --why false to inspect the obstacles.\n");
        for (size_t i = 0; i < info.step_count; ++i) {
            maelys_datalog_explanation_step_view_t s = {0};
            rc = maelys_datalog_prepared_explanation_step(prepared, i, &s);
            if (rc != MAELYS_DATALOG_STATUS_OK) goto fail;
            sentence(&out, "\nRule "); number(&out, s.rule_id); sentence(&out, " derives "); human_fact(&out, &s.fact); append(&out, "\n");
            out.rule_id = s.rule_id;
            for (size_t n = 0; n < s.premise_count; ++n) {
                maelys_datalog_explanation_premise_view_t p = {0};
                rc = maelys_datalog_prepared_explanation_premise(prepared, s.premise_begin + n, &p);
                if (rc == MAELYS_DATALOG_STATUS_OK) rc = human_premise(&out, &p);
                if (rc != MAELYS_DATALOG_STATUS_OK) goto fail;
            }
        }
    } else if (info.found) {
        sentence(&out, "The fact is present, so an absence explanation does not apply:\n");
        human_fact(&out, query); append(&out, "\n");
    } else {
        sentence(&out, "The fact is not derived: "); human_fact(&out, query); append(&out, "\n");
        if (info.false_summary == MAELYS_DATALOG_WHY_FALSE_SUMMARY_NO_CANDIDATE_RULE)
            sentence(&out, "No rule can derive this fact.\n");
        for (size_t i = 0; i < info.diagnostic_count; ++i) {
            maelys_datalog_explanation_obstacle_view_t o = {0};
            rc = maelys_datalog_prepared_explanation_obstacle(prepared, i, &o);
            if (rc == MAELYS_DATALOG_STATUS_OK) rc = human_obstacle(&out, &o);
            if (rc != MAELYS_DATALOG_STATUS_OK) goto fail;
        }
    }
    if (info.truncated) {
        sentence(&out, "\nThe explanation is incomplete: an exploration limit was reached.\n");
        if (info.kind == MAELYS_DATALOG_EXPLAIN_FALSE && !info.found)
            sentence(&out, "This does not prove that the fact cannot be derived.\n");
    }
    if (out.failed) {rc = MAELYS_DATALOG_STATUS_INTERNAL; goto fail;}
    *text = out.data; return MAELYS_DATALOG_STATUS_OK;
fail:
    free(out.data); return rc;
}
