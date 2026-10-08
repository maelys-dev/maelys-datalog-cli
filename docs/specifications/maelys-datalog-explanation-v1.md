# Structured CLI explanations

`explain` retains its existing `document` string and unchanged text output.
JSON data adds `structure`, described by `cli/schemas/explain.json`. Both
representations come from the same prepared explanation and retained result;
the CLI does not parse the text document to reconstruct proof records.

The common fields are `kind` (`why-true` or `why-false`), `found`, `truncated`,
`status`, the requested ground `query`, `steps`, and `obstacles`. Values retain
`symbol`, `integer`, or `boolean` kinds; patterns may additionally contain
`variable` terms. Integers stay integers and booleans stay booleans.

Why-true steps contain the derived fact, normalized `ruleId` and typed
`premises`. A premise identifies its kind, origin and body position. Positive
facts, negated absences and aggregate sources carry an `atom`; comparisons
carry an operator and typed operands; filters carry their program index and
value. Aggregates retain the SDK's unsigned `aggregateValue` and projected
variable. `parentStep` is a zero-based index in this explanation's `steps`, or
`null` when there is no parent proof step. A missing derivation has status
`not-derived`; a bounded proof may have status `truncated`.

Why-false obstacles contain the target, rule and depth, typed variable bindings
and already established supports. Each obstacle has its kind, origin and body
position, with a pattern, comparison or filter when applicable. The unbound
term mask identifies unresolved pattern positions. Status is `complete`,
`truncated`, or `not-applicable` when the query is present. `summary` records
`none` or `no-candidate-rule`. Search counts, `filterCostUnits`, named `limits`
and the original `limitHits` mask are present only for Why-false. They describe
bounded engine exploration, not time or memory measurements.

Step indices, variable IDs, normalized rule IDs and filter program indices
belong to the current explanation/program. They are not durable identifiers
across policy edits or SDK releases. An origin of `not-applicable` conveys no
fact origin, rather than implying a missing EDB or policy fact.

The CLI reserves one caller-owned arena for the requested kind before solving.
It keeps that arena and the result alive through text rendering and structured
reads, releases the prepared explanation first, then releases the result and
session. There is no duplicate session-owned explanation workspace. The
reference engine's preparation, accessors and text writing do not allocate;
the CLI's arena, text and JSON conversions still allocate. This is not a
whole-command zero-allocation or performance claim.
