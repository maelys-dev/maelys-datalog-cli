#!/bin/sh
# SPDX-License-Identifier: MPL-2.0
# Render one formula from the released tag's source archive and dependency pins.
set -eu
root=$(CDPATH='' cd -- "$(dirname "$0")/.." && pwd)
tag=${1:?usage: render-homebrew-formula.sh vX.Y.Z [OUTPUT [FORMULA]]}
output=${2:-$root/dist/homebrew/maelys-datalog.rb}
formula=${3:-maelys-datalog}
test "$formula" = maelys-datalog || { echo "unknown formula: $formula" >&2; exit 64; }
repository=${MAELYS_SOURCE_REPOSITORY:-maelys-dev/maelys-datalog-cli}
version=${tag#v}
url="https://github.com/$repository/archive/refs/tags/$tag.tar.gz"
temp_base=${TMPDIR:-/tmp}
work=$(mktemp -d "${temp_base%/}/maelys-datalog-formula.XXXXXX")
trap 'rm -rf "$work"' EXIT HUP INT TERM
curl -fsSL --retry 5 --retry-delay 3 -o "$work/source.tar.gz" "$url"
digest=$(shasum -a 256 "$work/source.tar.gz" | awk '{print $1}')
mkdir -p "$work/tag"
tar -xzf "$work/source.tar.gz" -C "$work/tag" --strip-components=1
test "$(cat "$work/tag/VERSION")" = "$version" || {
    echo "tag $tag does not carry VERSION $version" >&2; exit 1;
}
engine_tag=$(sed -n '1p' "$work/tag/dependencies/maelys-datalog.pin")
engine_pin=$(sed -n '2p' "$work/tag/dependencies/maelys-datalog.pin")
cli_tag=$(sed -n '1p' "$work/tag/dependencies/maelys-cli.pin")
cli_pin=$(sed -n '2p' "$work/tag/dependencies/maelys-cli.pin")
json_tag=$(sed -n '1p' "$work/tag/dependencies/maelys-json.pin")
json_pin=$(sed -n '2p' "$work/tag/dependencies/maelys-json.pin")
mkdir -p "$(dirname "$output")"
sed -e "s|@URL@|$url|g" -e "s|@VERSION@|$version|g" \
    -e "s|@SHA256@|$digest|g" \
    -e "s|@ENGINE_TAG@|$engine_tag|g" -e "s|@ENGINE_PIN@|$engine_pin|g" \
    -e "s|@CLI_TAG@|$cli_tag|g" -e "s|@CLI_PIN@|$cli_pin|g" \
    -e "s|@JSON_TAG@|$json_tag|g" -e "s|@JSON_PIN@|$json_pin|g" \
    "$work/tag/packaging/homebrew/maelys-datalog.rb.in" > "$output"
if grep -q '@[A-Z_]*@' "$output"; then
    echo "unrendered placeholder" >&2
    exit 1
fi
