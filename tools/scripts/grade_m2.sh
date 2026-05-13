#!/usr/bin/env bash
set -euo pipefail

required_files=(
  build/kernel.elf
  build/kernel.map
  build/inspect/readelf-header.txt
  build/inspect/readelf-program-headers.txt
  build/inspect/objdump-disassembly.txt
  build/inspect/nm-symbols.txt
  build/mcsos.iso
  build/mcsos.iso.sha256
  build/qemu-serial.log
)

for f in "${required_files[@]}"; do
  if [ ! -s "$f" ]; then
    echo "ERROR: artefak tidak ada atau kosong: $f" >&2
    exit 1
  fi
  echo "OK artifact: $f"
done

grep -q 'Class:.*ELF64' build/inspect/readelf-header.txt
grep -q 'Machine:.*Advanced Micro Devices X86-64' build/inspect/readelf-header.txt
grep -q 'Entry point address:.*0xffffffff80000000' build/inspect/readelf-header.txt
grep -q 'MCSOS 260502 M2 boot path entered' build/qemu-serial.log
grep -q '\[M2\] early serial online' build/qemu-serial.log
grep -q '\[M2\] kernel reached controlled halt loop' build/qemu-serial.log

echo "OK: M2 local grading checks passed"
