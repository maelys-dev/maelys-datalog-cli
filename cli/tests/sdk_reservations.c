/* SPDX-License-Identifier: MPL-2.0 */
#include <maelys/datalog_resources.h>
#include <maelys/datalog_program.h>
#include <maelys/datalog_explanations.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sdk_allocation_guard.h"
#undef malloc
#undef calloc
#undef realloc
#undef free
static int forbidden;
static size_t attempts;
void *cli_sdk_malloc(size_t n) { if(forbidden) {++attempts;return NULL;} return malloc(n); }
void *cli_sdk_calloc(size_t n,size_t s) { if(forbidden) {++attempts;return NULL;} return calloc(n,s); }
void *cli_sdk_realloc(void *p,size_t n) { if(forbidden) {++attempts;return NULL;} return realloc(p,n); }
void cli_sdk_free(void *p) { if(forbidden) ++attempts; free(p); }
#define OK(c) assert((c)==MAELYS_DATALOG_STATUS_OK)
static maelys_datalog_session_t *create(maelys_datalog_policy_t *p,unsigned kinds,int inspect,maelys_datalog_session_storage_plan_t *plan) {
 maelys_datalog_session_config_t *c; maelys_datalog_session_t *s;
 OK(maelys_datalog_session_config_create(&c));
 if(kinds) OK(maelys_datalog_session_config_set_explanation_workspace(c,kinds));
 if(inspect) {
  maelys_datalog_session_resource_request_t r=MAELYS_DATALOG_RESOURCE_REQUEST_INIT;
  r.required_features=MAELYS_DATALOG_RESOURCE_SESSION_CAPACITIES;
  r.capacity_mask=MAELYS_DATALOG_CAPACITY_INPUT_FACTS|MAELYS_DATALOG_CAPACITY_DERIVED_FACTS;
  OK(maelys_datalog_session_config_set_resources(c,&r));
 }
 OK(maelys_datalog_session_storage_requirements_configured(p,0,c,plan,NULL));
 OK(maelys_datalog_session_create_configured(p,0,c,&s));
 OK(maelys_datalog_session_config_free(c));return s;
}
static maelys_datalog_status_t text(maelys_datalog_result_t *r,unsigned k,const maelys_datalog_value_t *v,char *out,size_t n,size_t *required) {
 return k==MAELYS_DATALOG_EXPLAIN_TRUE ? maelys_datalog_result_explain_true_text(r,"seen",v,1,out,n,required) : maelys_datalog_result_explain_false_text(r,"seen",v,1,out,n,required);
}
int main(void) {
 const maelys_datalog_predicate_t preds[]={MAELYS_DATALOG_EDB("seed",1),MAELYS_DATALOG_IDB_QUERY("seen",1)};
 const maelys_datalog_domain_t domain={"reservations",preds,2,NULL,0};
 OK(maelys_datalog_domain_register(&domain)); maelys_datalog_policy_t *p;
 const char *source="seen(X) :- seed(X).";
 OK(maelys_datalog_policy_load_inline(domain.name,"p",source,strlen(source),&p,NULL));
 maelys_datalog_session_storage_plan_t plans[4]; maelys_datalog_session_t *sessions[4];
 const unsigned kinds[]={0,MAELYS_DATALOG_EXPLAIN_TRUE,MAELYS_DATALOG_EXPLAIN_FALSE,MAELYS_DATALOG_EXPLAIN_TRUE|MAELYS_DATALOG_EXPLAIN_FALSE};
 for(size_t i=0;i<4;++i) {plans[i]=(maelys_datalog_session_storage_plan_t)MAELYS_DATALOG_SESSION_PLAN_INIT;sessions[i]=create(p,kinds[i],0,&plans[i]);}
 assert(!plans[0].explanation_bytes);
 assert(plans[1].explanation_bytes < plans[3].explanation_bytes);
 assert(plans[2].explanation_bytes == plans[3].explanation_bytes);
 maelys_datalog_session_storage_plan_t zero=MAELYS_DATALOG_SESSION_PLAN_INIT;
 maelys_datalog_session_t *inspection=create(p,0,1,&zero);
 assert(zero.arena_bytes < plans[0].arena_bytes);
 maelys_datalog_session_resources_t resources=MAELYS_DATALOG_RESOURCES_INIT;
 maelys_datalog_session_resources_t defaults=MAELYS_DATALOG_RESOURCES_INIT;
 const maelys_datalog_program_t *a,*b; maelys_datalog_program_info_t ai,bi;
 forbidden=1;
 OK(maelys_datalog_session_get_resources(inspection,&resources));
 assert(!resources.input_facts && !resources.derived_facts);
 OK(maelys_datalog_session_get_resources(sessions[0],&defaults));
 assert(resources.symbols==defaults.symbols && resources.text_bytes==defaults.text_bytes);
 OK(maelys_datalog_session_program(sessions[0],&a)); OK(maelys_datalog_session_program(inspection,&b));
 OK(maelys_datalog_program_info(a,&ai)); OK(maelys_datalog_program_info(b,&bi));
 assert(ai.max_input_facts==bi.max_input_facts && ai.max_derived_facts==bi.max_derived_facts && ai.max_facts_per_predicate==bi.max_facts_per_predicate);
 assert(ai.rule_count==bi.rule_count && ai.predicate_count==bi.predicate_count && ai.fact_count==bi.fact_count);
 char af[65],bf[65]; OK(maelys_datalog_program_fingerprint(a,af));OK(maelys_datalog_program_fingerprint(b,bf));assert(!strcmp(af,bf));
 size_t ac,bc;OK(maelys_datalog_program_query_count(a,&ac));OK(maelys_datalog_program_query_count(b,&bc));assert(ac==bc && ac==1);
 maelys_datalog_predicate_t aq,bq;OK(maelys_datalog_program_query(a,0,&aq));OK(maelys_datalog_program_query(b,0,&bq));assert(aq.arity==bq.arity && !strcmp(aq.name,bq.name));
 forbidden=0;
 for(size_t i=1;i<3;++i) {
  maelys_datalog_value_t v={.kind=MAELYS_DATALOG_VALUE_INTEGER,.as.integer=42};
  maelys_datalog_fact_t fact={.predicate="seed",.arity=1};fact.terms[0]=v;
  maelys_datalog_result_t *r;size_t required=0;char buffer[4096],short_buffer[1];
  size_t arena_bytes,arena_alignment;
  OK(maelys_datalog_session_explanation_storage_bound(sessions[i],(maelys_datalog_explanation_kind_t)kinds[i],&arena_bytes,&arena_alignment));
  void *arena=malloc(arena_bytes);assert(arena && arena_alignment<=_Alignof(max_align_t));
  for(int pass=0;pass<2;++pass) {
   forbidden=1;
   OK(maelys_datalog_session_solve(sessions[i],i==1?&fact:NULL,i==1?1:0,&r,NULL));
   OK(text(r,kinds[i],&v,NULL,0,&required));assert(required<sizeof buffer);
   assert(text(r,kinds[i],&v,short_buffer,sizeof short_buffer,&required)==MAELYS_DATALOG_STATUS_PAYLOAD_TOO_LARGE && !short_buffer[0]);
   OK(text(r,kinds[i],&v,buffer,sizeof buffer,&required));assert(strlen(buffer)==required);
   size_t untouched=123;
   assert(text(r,kinds[i]==MAELYS_DATALOG_EXPLAIN_TRUE?MAELYS_DATALOG_EXPLAIN_FALSE:MAELYS_DATALOG_EXPLAIN_TRUE,&v,NULL,0,&untouched)==MAELYS_DATALOG_STATUS_UNSUPPORTED && untouched==123);
   maelys_datalog_prepared_explanation_t *prepared=NULL;
   OK(maelys_datalog_result_prepare_explanation(r,(maelys_datalog_explanation_kind_t)kinds[i],"seen",&v,1,arena,arena_bytes,&prepared));
   assert(maelys_datalog_result_free(r)==MAELYS_DATALOG_STATUS_INVALID_STATE);
   OK(maelys_datalog_prepared_explanation_release(prepared));
   OK(maelys_datalog_result_free(r));forbidden=0;
  }
  free(arena);
 }
#ifdef CLI_SDK_ALLOC_GUARDED
 assert(!attempts);
#endif
 printf("arena bytes: default=%zu true=%zu false=%zu both=%zu inspection=%zu; explanation bytes: true=%zu false=%zu; allocation guard compiled=%d, attempts=%zu\n",plans[0].arena_bytes,plans[1].arena_bytes,plans[2].arena_bytes,plans[3].arena_bytes,zero.arena_bytes,plans[1].explanation_bytes,plans[2].explanation_bytes,
#ifdef CLI_SDK_ALLOC_GUARDED
 1,
#else
 0,
#endif
 attempts);
 for(size_t i=0;i<4;++i) {
  OK(maelys_datalog_session_free(sessions[i]));
 }
 OK(maelys_datalog_session_free(inspection));
 OK(maelys_datalog_policy_free(p));
}
