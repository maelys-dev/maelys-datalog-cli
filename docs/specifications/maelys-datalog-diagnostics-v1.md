# CLI failure diagnostics

Operational failures retain their agent-cli/v2 `error.code` and exit 1. Product
failures additionally carry `error.issues`, with a stable `code`, canonical
English `message`, and optional `path` supplied by the caller. Framework parser
failures keep the framework's envelope and need not carry product detail.

| Issue code | Cause |
| --- | --- |
| DATALOG_DOMAIN | Reading or validating a domain declaration |
| DATALOG_FACTS | Reading or validating the input fact batch |
| DATALOG_ENGINE | Failure reported by the public SDK |
| DATALOG_QUERY_UNKNOWN | Unknown or unauthorized query predicate |
| DATALOG_QUERY_ARITY | Query term count differs from predicate arity |
| DATALOG_QUERY_TERM | A query operand is not a valid typed term |
| DATALOG_REQUEST | Other application-level request or runtime failure |

`error["x-datalog"]` carries reader `index` (zero-based), `line` and `column`
(one-based) when available. Missing locations are omitted, not reconstructed
from a message. Reader `path` is the provided input path, not a resolved path.
The SDK's versioned public diagnostic is copied under `diagnostic`, with the
same conditional fields as `check` diagnostics; its `present` mask remains the
authority on which SDK fields are meaningful. No private engine layout is read.

Semantic rejection by `check` retains its existing successful envelope,
`data.valid=false`, `data.diagnostics` and exit 2. It is not turned into an
operational failure. Textual engine explanation documents remain unchanged.
Messages are human descriptions, not identifiers to parse; use issue codes and
structured fields. JSON strings, including paths containing quotes, are escaped
by the framework writer. These conversions allocate in the CLI.
