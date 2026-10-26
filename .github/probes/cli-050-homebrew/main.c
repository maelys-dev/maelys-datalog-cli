#include <maelys/datalog.h>
#include <maelys/datalog_builders.h>

#include <stdio.h>
#include <string.h>

static int ok(maelys_datalog_status_t status, const char *step) {
    if (status == MAELYS_DATALOG_STATUS_OK) return 1;
    fprintf(stderr, "%s: %s\n", step, maelys_datalog_status_name(status));
    return 0;
}

static int expect(
    const maelys_datalog_result_t *result,
    const char *predicate, const char *first, const char *second,
    int expected) {
    const maelys_datalog_value_t terms[] = {
        MAELYS_DATALOG_SYMBOL(first),
        MAELYS_DATALOG_SYMBOL(second),
    };

    int present = 0;
    size_t arity = second == NULL ? 1u : 2u;
    if (!ok(maelys_datalog_result_query(
            result, predicate, terms, arity, &present), "query")) {
        return 0;
    }
    printf("%s(%s", predicate, first);
    if (second != NULL) printf(", %s", second);
    printf(") = %s\n", present ? "true" : "false");
    return present == expected;
}

int main(void) {
    static const maelys_datalog_predicate_t predicates[] = {
        MAELYS_DATALOG_EDB("user", 1),
        MAELYS_DATALOG_EDB("owns", 2),
        MAELYS_DATALOG_EDB("delegated", 2),
        MAELYS_DATALOG_EDB("blocked", 1),
        MAELYS_DATALOG_IDB("can_read", 2),
        MAELYS_DATALOG_IDB_QUERY("has_any_document", 1),
        MAELYS_DATALOG_IDB_QUERY("allow", 2),
    };
    const maelys_datalog_domain_t domain = {
        "documents", predicates,
        sizeof(predicates) / sizeof(predicates[0]), NULL, 0u,
    };
    const char source[] =
        "can_read(User, Doc) :-\n"
        "    owns(User, Doc) or delegated(User, Doc),\n"
        "    not(blocked(User)).\n"
        "has_any_document(User) :- owns(User, _).\n"
        "allow(User, Doc) :- user(User), can_read(User, Doc).\n";

    maelys_datalog_policy_t *policy = NULL;
    maelys_datalog_session_t *session = NULL;
    maelys_datalog_session_config_t *config = NULL;
    maelys_datalog_result_t *result = NULL;
    maelys_datalog_input_edb_t *edb = NULL;
    maelys_datalog_diagnostic_t diagnostic = MAELYS_DATALOG_DIAGNOSTIC_INIT;
    int exit_code = 1;

    if (!ok(maelys_datalog_domain_register(&domain), "register domain"))
        goto cleanup;

    maelys_datalog_diagnostic_clear(&diagnostic);
    if (!ok(maelys_datalog_policy_load_inline(
            domain.name, "documents.main", source, strlen(source),
            &policy, &diagnostic), "load policy")) {
        if (diagnostic.message[0] != '\0')
            fprintf(stderr, "  %s\n", diagnostic.message);
        goto cleanup;
    }

    if (!ok(maelys_datalog_session_config_create(&config), "create config"))
        goto cleanup;
    if (!ok(maelys_datalog_session_config_set_explanation_workspace(
            config, MAELYS_DATALOG_EXPLAIN_TRUE | MAELYS_DATALOG_EXPLAIN_FALSE),
            "reserve explanation workspace"))
        goto cleanup;
    if (!ok(maelys_datalog_session_create_configured(policy, 0u, config, &session),
            "create session"))
        goto cleanup;
    if (!ok(maelys_datalog_session_config_free(config), "free config"))
        goto cleanup;
    config = NULL;

    /* The prepared session owns its state; the policy handle may be freed. */
    if (!ok(maelys_datalog_policy_free(policy), "free policy"))
        goto cleanup;
    policy = NULL;

    if (!ok(maelys_datalog_input_edb_create(&edb), "create input EDB")) goto cleanup;
    if (!ok(MAELYS_DATALOG_ADD_FACTS(
        edb, &diagnostic,
        MAELYS_DATALOG_FACT("user", "alice"),
        MAELYS_DATALOG_FACT("user", "bob"),
        MAELYS_DATALOG_FACT("user", "mallory"),
        MAELYS_DATALOG_FACT("owns", "alice", "roadmap.pdf"),
        MAELYS_DATALOG_FACT("delegated", "bob", "roadmap.pdf"),
        MAELYS_DATALOG_FACT("owns", "mallory", "roadmap.pdf"),
        MAELYS_DATALOG_FACT("blocked", "mallory")
    ), "add request facts")) {
        if (diagnostic.message[0] != '\0')
            fprintf(stderr, "  %s\n", diagnostic.message);
        goto cleanup;
    }

    maelys_datalog_diagnostic_clear(&diagnostic);
    if (!ok(maelys_datalog_session_solve_edb(
            session, edb,
            &result, &diagnostic), "solve")) {
        if (diagnostic.message[0] != '\0')
            fprintf(stderr, "  %s\n", diagnostic.message);
        goto cleanup;
    }

    int answers_match = 1;
    answers_match &= expect(result, "allow", "alice", "roadmap.pdf", 1);
    answers_match &= expect(result, "allow", "bob", "roadmap.pdf", 1);
    answers_match &= expect(result, "allow", "mallory", "roadmap.pdf", 0);
    answers_match &= expect(result, "has_any_document", "alice", NULL, 1);
    answers_match &= expect(result, "has_any_document", "bob", NULL, 0);
    exit_code = answers_match ? 0 : 2;

cleanup:
    if (config != NULL) (void)maelys_datalog_session_config_free(config);
    (void)maelys_datalog_input_edb_free(edb);
    if (result != NULL) (void)maelys_datalog_result_free(result);
    if (session != NULL) (void)maelys_datalog_session_free(session);
    if (policy != NULL) (void)maelys_datalog_policy_free(policy);
    return exit_code;
}