# Maelys Datalog CLI

`maelys-datalog` is a command-line consumer of the public Maelys Datalog SDK.
It validates policies and domains, reports fingerprints, solves a batch of
facts, and renders Why-true or Why-false explanations. The engine is a pinned
dependency at the v0.22.0 compatibility freeze; this repository does not contain or modify its sources.

The command follows [agent-cli/v2](docs/cli.md). Run
`maelys-datalog describe --summary --format json` for its machine-readable
catalog. JSON explanations also include a [typed structure](docs/specifications/maelys-datalog-explanation-v1.md)
while preserving their text document. The [domain declaration](docs/specifications/maelys-datalog-domain-v1.md)
is a strict JSON file parsed with `maelys-json`.

```sh
maelys-datalog check --domain cli/tests/fixtures/rbac.domain.json \
  cli/tests/fixtures/rbac.dl --format json
maelys-datalog queries --domain cli/tests/fixtures/rbac.domain.json \
  cli/tests/fixtures/rbac.dl
maelys-datalog solve --domain cli/tests/fixtures/rbac.domain.json \
  --facts cli/tests/fixtures/rbac.facts.dl cli/tests/fixtures/rbac.dl
maelys-datalog explain --domain cli/tests/fixtures/rbac.domain.json \
  --facts cli/tests/fixtures/rbac.facts.dl --why false \
  cli/tests/fixtures/rbac.dl can_deliver '"Mallory"' --format json
```

`check` and `queries` also accept `--manifest FILE` and repeated `--domain`
declarations for multi-domain policy sets. `check` reports each enabled policy's
identifier and normalized rule count. A manifest with no enabled policies is
valid and reports zero policies and the SDK's deterministic policy fingerprint,
including for an empty set. `queries` lists each enabled policy's domain
and effectively authorized predicate/arity pairs, applying the manifest's
whitelist; an absent or empty whitelist authorizes no queries. The command
prepares one session at a time for inspection and does not solve facts.
`fingerprint` reports policy and execution
identities. In text mode, `solve` prints each selected query's facts in the
engine's canonical order, including a heading for empty results. In JSON mode,
typed terms remain JSON strings, exact int64 numbers, or booleans. JavaScript
clients need a precision-preserving JSON parser above 2^53.
Quoted symbols in CLI fact files and typed `explain` operands accept JSON
string escapes, including Unicode surrogate pairs; `solve` prints symbols
with the same escaping. This input notation does not change the engine's
policy source language.

Install the published command from the Maelys Homebrew tap:

```sh
brew install maelys-dev/tap/maelys-datalog
```

To build from source, materialize the pinned checkouts and export the line
printed by the generated checkout script:

```sh
sh scripts/checkout-dependencies.sh /tmp/maelys-datalog-cli-dependencies
export MAELYS_DEPENDENCIES_DIR=/tmp/maelys-datalog-cli-dependencies
make all
make check
make asan-cli PROFILE=SMALL
```

The binary is `build/bin/maelys-datalog`. The build installs the pinned SDK
into its private build tree and compiles the command only against its public
installed headers and archive. The CLI and its JSON reader allocate memory;
the engine's allocation guarantees do not apply to those application layers.

`inspect` reports each enabled compiled policy’s counts, authorized queries and
program limits. `inspectionCapacity` describes the temporary inspection session
(input and derived capacities are zero), not the limits of a solving session.
These counts and capacities do not measure RSS or execution speed.
