# Changelog

All notable changes to `maelys-datalog-cli` are documented here.

## [Unreleased]

### Changed

- Reserve only the requested Why-true or Why-false workspace for `explain`.
  Prepare `queries` sessions with zero input/derived fact capacities while
  retaining the compiled program's limits and authorized query surface.
- Add installed-SDK reservation, explanation lease/retry and allocator-disabled
  replay checks in SMALL and LARGE. Reservation sizes are not RSS or timings.

- Adopt the published v0.22.0 compatibility-freeze SDK. Preserve the explicit
  unsupported nonzero work-limit diagnostic for the reference backend.
- Replay caller-owned policy liveness and session lifetime against installed
  public SDK headers: released policy accessors return `INVALID_STATE` without
  changing outputs, while an existing session survives arena reuse.

## 0.3.1 — 2026-10-03

### Fixed

- Explain that a nonzero `--work-limit` requires the backend's `WORK_LIMIT`
  capability, which the reference backend does not support. Keep the
  `UNSUPPORTED` error code and exit status 1 in text and JSON reports, and
  document the restriction in the generated command help and reference.

## 0.3.0 — 2026-10-03

### Changed

- Pin `maelys-datalog` to v0.21.0, `maelys-cli` to v0.5.33 and the
  `maelys-release` socle to v0.62.3. The `maelys-json` v0.2.0 and
  `agent-cli-spec` v2.6.0 pins remain the latest published versions.
- Adopt the SDK's live empty-set contract directly. `check` and `queries`
  now return the deterministic engine policy fingerprint for an empty manifest
  instead of `null`; their data schemas require a fingerprint string. Remove
  the v0.20.0 empty-count compatibility probe. Nonempty policy identities and
  text output keep their existing behavior.
- Regenerate the CLI reference and release workflows with the updated socle.
  Homebrew bottle jobs now run the formula's tests on the poured bottle before
  publishing the formula to the tap.

## 0.2.0 — 2026-10-03

### Added

- Add `queries` to inspect the effectively authorized predicate/arity pairs of
  each enabled policy, including multi-domain manifests and empty whitelists,
  without solving facts. Text and JSON reports identify the policy and domain.
- Include enabled policy identifiers in `check` JSON reports.

### Changed

- Pin the `maelys-datalog` engine SDK to v0.20.0 and remove the obsolete engine
  CLI CMake option.
  Read-only program inspection uses the installed public `datalog_program.h`;
  provider and module headers remain forbidden.

### Fixed

- Accept successful manifests with no enabled policies in the `check` schema
  and report zero policies and rules with a null fingerprint, since the SDK
  does not fingerprint empty sets. The engine's empty policy handles are
  released normally.

## 0.1.1 — 2026-10-01

### Fixed

- Return success after rendering a complete Homebrew formula and keep its
  installation test within Homebrew's style limit, so the tap job can build
  bottles for the first CLI release.

## 0.1.0 — 2026-10-01

### Added

- Introduce the standalone `maelys-datalog` CLI with validation, fingerprints,
  solving and explanations on the v0.17.0 engine SDK.
- Publish the `maelys-datalog-domain-v1` JSON declaration and agent-cli/v2
  command schemas, with a Homebrew command formula and generated CLI reference.

### Fixed

- Print query facts and both fingerprints in text mode; report missing input
  files and invalid arguments with their proper CLI error codes.
- Accept escaped symbols in fact files and explanation queries, enforce fact
  arity in the solve schema, and derive the command version from `VERSION`.

## 0.0.0 — 2026-10-01

Repository initialized for the standalone command.
