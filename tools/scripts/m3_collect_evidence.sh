#!/usr/bin/env bash
set -Eeuo pipefail

OUT="${1:-evidence/M3}"
mkdir -p "$OUT"
cp -v build/kernel.elf "$OUT/" 2>/dev/null || true
cp -v build/kernel.map "$OUT/" 2>/dev/null || true
cp -v build/kernel.readelf.header.txt "$OUT/" 2>/dev/null || true
cp -v build/kernel.readelf.programs.txt "$OUT/" 2>/dev/null || true
cp -v build/kernel.disasm.txt "$OUT/" 2>/dev/null || true
cp -v build/kernel.syms.txt "$OUT/" 2>/dev/null || true
cp -v build/m3_serial.log "$OUT/" 2>/dev/null || true
cp -v build/m3_audit_readelf_header.txt "$OUT/" 2>/dev/null || true
cp -v build/m3_audit_readelf_programs.txt "$OUT/" 2>/dev/null || true
cp -v build/m3_audit_symbols.txt "$OUT/" 2>/dev/null || true
cp -v build/m3_audit_disasm.txt "$OUT/" 2>/dev/null || true
{
  echo "# M3 evidence manifest"
  date -u +"generated_utc=%Y-%m-%dT%H:%M:%SZ"
  git rev-parse HEAD 2>/dev/null | sed 's/^/commit=/g' || true
  clang --version | head -n 1 | sed 's/^/clang=/g'
  ld.lld --version | head -n 1 | sed 's/^/lld=/g'
  qemu-system-x86_64 --version 2>/dev/null | head -n 1 | sed 's/^/qemu=/g' || true
  find "$OUT" -maxdepth 1 -type f -printf '%f\n' | LC_ALL=C sort
} > "$OUT/manifest.txt"
echo "PASS: evidence tersimpan di $OUT"
