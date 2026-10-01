#!/bin/sh
# SPDX-License-Identifier: MPL-2.0
set -eu
root=$(CDPATH='' cd -- "$(dirname "$0")/.." && pwd)
cd "$root"
case ${1:?usage: scripts/verify-release.sh TARGET} in
    linux-x86_64|linux-arm64|macos-arm64) ;;
    *) echo "unsupported target" >&2; exit 64 ;;
esac
make BUILD_DIR=build/release check
test -f docs/cli.md
test -f docs/cli-contract.json
