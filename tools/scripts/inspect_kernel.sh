#!/usr/bin/env bash
set -euo pipefail

KERNEL="build/kernel.elf"
OUT="build/inspect"
mkdir -p "$OUT"

test -f "$KERNEL"

readelf -hW "$KERNEL" | tee "$OUT/readelf-header.txt"
readelf -lW "$KERNEL" | tee "$OUT/readelf-program-headers.txt"
readelf -SW "$KERNEL" | tee "$OUT/readelf-sections.txt" >/dev/null
objdump -drwC "$KERNEL" | tee "$OUT/objdump-disassembly.txt" >/dev/null
nm -n "$KERNEL" | tee "$OUT/nm-symbols.txt" >/dev/null

 grep -q 'Class:.*ELF64' "$OUT/readelf-header.txt"
 grep -q 'Machine:.*Advanced Micro Devices X86-64' "$OUT/readelf-header.txt"
 grep -q 'Entry point address:.*0xffffffff80000000' "$OUT/readelf-header.txt"
 grep -q 'kmain' "$OUT/nm-symbols.txt"
 grep -q 'serial_init' "$OUT/nm-symbols.txt"
 grep -q 'serial_write' "$OUT/nm-symbols.txt"

echo "OK: kernel ELF inspection passed"
