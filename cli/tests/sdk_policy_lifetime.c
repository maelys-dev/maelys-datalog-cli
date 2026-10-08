/* SPDX-License-Identifier: MPL-2.0 */
/* Consumer replay against installed public SDK headers only. */
#include <maelys/datalog_extension.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
int main(void) {
 const maelys_datalog_predicate_t preds[] = {MAELYS_DATALOG_EDB("seed",1),MAELYS_DATALOG_IDB_QUERY("seen",1)};
 const maelys_datalog_domain_t domain={"audit",preds,2,NULL,0};
 assert(!maelys_datalog_domain_register(&domain));
 size_t bytes,alignment; void *storage=NULL;
 assert(!maelys_datalog_policy_storage_requirements(&bytes,&alignment));
 if(alignment<sizeof(void*)) alignment=sizeof(void*);
 assert(!posix_memalign(&storage,alignment,bytes));
 maelys_datalog_policy_t *policy=NULL;
 const char *source="seen(X) :- seed(X).";
 assert(!maelys_datalog_policy_load_frontend_in(storage,bytes,"audit","p",source,strlen(source),maelys_datalog_frontend_datalog(),&policy,NULL));
 maelys_datalog_session_t *session=NULL;
 assert(!maelys_datalog_session_create(policy,0,&session));
 assert(!maelys_datalog_policy_free(policy));
 size_t count=123,stat=456; const char *id="unchanged"; const char *sentinel=id;
 char fingerprint[MAELYS_DATALOG_PUBLIC_FINGERPRINT_BYTES],expected[sizeof fingerprint];
 memset(fingerprint,'x',sizeof fingerprint); memcpy(expected,fingerprint,sizeof fingerprint);
 assert(maelys_datalog_policy_count(policy,&count)==MAELYS_DATALOG_STATUS_INVALID_STATE && count==123);
 assert(maelys_datalog_policy_id(policy,0,&id)==MAELYS_DATALOG_STATUS_INVALID_STATE && id==sentinel);
 assert(maelys_datalog_policy_stat_get(policy,0,MAELYS_DATALOG_POLICY_RULE_COUNT,&stat)==MAELYS_DATALOG_STATUS_INVALID_STATE && stat==456);
 assert(maelys_datalog_policy_fingerprint(policy,fingerprint)==MAELYS_DATALOG_STATUS_INVALID_STATE && !memcmp(fingerprint,expected,sizeof fingerprint));
 maelys_datalog_session_t *other=NULL;
 assert(maelys_datalog_session_create(policy,0,&other)==MAELYS_DATALOG_STATUS_INVALID_STATE && !other);
 memset(storage,0xa5,bytes); free(storage);
 maelys_datalog_fact_t fact={.predicate="seed",.arity=1};
 fact.terms[0].kind=MAELYS_DATALOG_VALUE_INTEGER; fact.terms[0].as.integer=42;
 maelys_datalog_result_t *result=NULL; int found=0;
 assert(!maelys_datalog_session_solve(session,&fact,1,&result,NULL));
 assert(!maelys_datalog_result_query(result,"seen",fact.terms,1,&found) && found);
 assert(!maelys_datalog_result_free(result)); assert(!maelys_datalog_session_free(session));
 puts("policy accessors: INVALID_STATE and unchanged outputs; borrowing session after arena reuse: PASS");
}
