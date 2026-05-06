#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUT="$ROOT/build/repro"
mkdir -p "$OUT"

rm -rf "$ROOT/build/proof"
"$ROOT/tools/scripts/proof_compile.sh" > /dev/null
sha256sum "$ROOT/build/proof/freestanding_probe.o" \
"$ROOT/build/proof/freestanding_probe.elf" | tee "$OUT/sha256-run1.txt"

rm -rf "$ROOT/build/proof"
"$ROOT/tools/scripts/proof_compile.sh" > /dev/null
sha256sum "$ROOT/build/proof/freestanding_probe.o" \
"$ROOT/build/proof/freestanding_probe.elf" | tee "$OUT/sha256-run2.txt"

if diff -u "$OUT/sha256-run1.txt" "$OUT/sha256-run2.txt" > "$OUT/sha256-diff.txt"; then
    echo "OK: proof build is reproducible for M1 inputs" | tee "$OUT/repro-status.txt"
else
    echo "ERROR: proof build hash differs; inspect $OUT/sha256-diff.txt" | tee "$OUT/repro-status.txt" >&2
    exit 1
fi
