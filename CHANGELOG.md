# Changelog

All notable changes to `maelys-datalog-cli` are documented here.

## [Unreleased]

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
