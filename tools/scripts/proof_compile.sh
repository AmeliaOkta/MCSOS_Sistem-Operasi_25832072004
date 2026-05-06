#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUT="$ROOT/build/proof"
SRC="$ROOT/tests/toolchain/freestanding_probe.c"
mkdir -p "$OUT"

CFLAGS=(
    --target=x86_64-unknown-elf
    -std=c17
    -ffreestanding
    -fno-stack-protector
    -fno-pic
    -mno-red-zone
    -mno-mmx
    -mno-sse
    -mno-sse2
    -Wall
    -Wextra
    -Werror
    -O2
    -c
)

clang "${CFLAGS[@]}" "$SRC" -o "$OUT/freestanding_probe.o"

ld.lld \
    -m elf_x86_64 \
    -nostdlib \
    --entry=mcsos_toolchain_probe \
    -Ttext=0xffffffff80000000 \
    -o "$OUT/freestanding_probe.elf" \
    "$OUT/freestanding_probe.o"

readelf -hW "$OUT/freestanding_probe.o" | tee "$OUT/readelf-object-header.txt"
readelf -hW "$OUT/freestanding_probe.elf" | tee "$OUT/readelf-header.txt"
readelf -SW "$OUT/freestanding_probe.elf" | tee "$OUT/readelf-sections.txt"
objdump -drwC "$OUT/freestanding_probe.o" | tee "$OUT/objdump-disassembly.txt"
nm -u "$OUT/freestanding_probe.elf" | tee "$OUT/nm-undefined.txt"
file "$OUT/freestanding_probe.o" "$OUT/freestanding_probe.elf" | tee "$OUT/file-type.txt"

if [ -s "$OUT/nm-undefined.txt" ]; then
    echo "ERROR: undefined symbols detected in freestanding ELF" >&2
    exit 1
fi

echo "OK: freestanding x86_64 ELF proof generated"
