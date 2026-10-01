# Changelog

All notable changes to `maelys-datalog-cli` are documented here.

## [Unreleased]

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
