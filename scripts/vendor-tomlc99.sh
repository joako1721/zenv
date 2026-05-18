#!/usr/bin/env sh
# Vendor tomlc99 into third_party/tomlc99/. Run once after cloning.
set -eu

DEST=third_party/tomlc99
REV="${TOMLC99_REV:-master}"
URL_BASE="https://raw.githubusercontent.com/cktan/tomlc99/$REV"

mkdir -p "$DEST"
for f in toml.c toml.h LICENSE README.md; do
	echo "fetching $f"
	curl -fsSL "$URL_BASE/$f" -o "$DEST/$f"
done
echo "vendored tomlc99 @ $REV into $DEST"
