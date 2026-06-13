#!/usr/bin/env bash
set -uo pipefail

for d in include src kernel tests tools configs; do
  echo "================ STRUKTUR $d/ ================"
  find "$d" -type f 2>/dev/null
  echo
done

echo "================ DOCS (2 level) ================"
find docs -maxdepth 2 -type f 2>/dev/null

echo; echo "================ MAKEFILE TARGETS ================"
grep -nE "^[A-Za-z0-9_./-]+:" Makefile

echo; echo "================ GREP API PENTING (exclude limine/third_party/build/iso_root) ================"
for sym in pmm_alloc_frame pmm_free_frame x86_64_trap_dispatch idt_init lidt serial_init serial_write panic pic_send_eoi timer_; do
  echo "--- $sym ---"
  grep -RIn "$sym" . \
    --include="*.c" --include="*.h" --include="*.S" --include="*.s" \
    --exclude-dir=limine --exclude-dir=third_party --exclude-dir=build --exclude-dir=iso_root \
    2>/dev/null | head -5
done
