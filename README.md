# Maelys Datalog CLI

`maelys-datalog` is a command-line consumer of the public Maelys Datalog SDK.
It validates policies and domains, reports fingerprints, solves a batch of
facts, and renders Why-true or Why-false explanations. The engine is a pinned
dependency pinned by tag and commit in `dependencies/maelys-datalog.pin`,
following the compatibility freeze introduced in v0.22.0; this repository does not contain or modify its sources.

The command follows [agent-cli/v2](docs/cli.md). Run
`maelys-datalog describe --summary --format json` for its machine-readable
catalog. JSON explanations also include a [typed structure](docs/specifications/maelys-datalog-explanation-v1.md)
while preserving their text document. The [domain declaration](docs/specifications/maelys-datalog-domain-v1.md)
is a strict JSON file parsed with `maelys-json`.

The examples below and in `help COMMAND` use paths relative to the root of the
CLI source checkout. Homebrew does not install these test fixtures; use your own
domain, policy and fact files when working outside that checkout.

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

`check`, `inspect` and `queries` also accept `--manifest FILE` and repeated `--domain`
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

Homebrew builds statically against `maelys-dev/tap/libmaelys-datalog`; installed
CLI bottles need no separate engine library at runtime. Source builds still use
the verified engine tag and commit. To consume an existing SDK instead, set
`INSTALLED_SDK_PREFIX` to its installation prefix. Its pkg-config version must
be at least the engine pin, and its installed public headers must declare API 2
and program ABI 2. Its loaded profile must match `PROFILE` (SMALL by
default). The prefix is read only: missing archives, older versions, different contracts or profile
mismatches fail rather than triggering a source build there. This checks package
version, compiled public contract and loaded profile, not cryptographic provenance of a supplied archive.
A newer stable Homebrew SDK is accepted within that frozen contract; source
builds still use the exact tag and commit declared by the pin.

Failures preserve their general agent-cli code and carry [structured causes](docs/specifications/maelys-datalog-diagnostics-v1.md) for agents and editors.

`man maelys-datalog` is installed with the CLI, generated from the catalog.
`make manual-mdx` renders the same manual into `build/man/maelys-datalog.mdx`
for a documentation site; neither rendering is maintained by hand.

Human help and CLI diagnostics support `--lang fr`, `MAELYS_CLI_LANG` and locale
selection. English remains canonical; JSON identifiers, messages and structured
data retain their canonical bytes (the rendered `help` text follows the selected
language, including inside a JSON envelope). Known explanation diagnostic
messages and hints are translated for human text; the SDK diagnostic in JSON
and engine explanation documents in JSON are preserved verbatim. Default
`explain` text presents facts, variable bindings and obstacles in readable English
or French. Named variables appear with their source spelling (`User`, `R`), read
from the SDK (0.27.0 or later); a variable the SDK does not name, such as an
anonymous `_`, keeps its rule-local `?N` identifier.
`explain --raw` prints the original SDK document for inspection.
Both English and French manuals are installed.

`make invocation-check` runs the pinned contract's catalog-generated invocation
campaign, also included in `make check`: fixed boundary cases plus eight generated
lines per command, canonical/default JSON formats, a ten-second per-process timeout
and a 180-second campaign budget. Each line gets an isolated directory and home;
`--apply` is not enabled. An incomplete campaign fails rather than counting as a
pass. Its line count is separate from the conformance kit's control count.
