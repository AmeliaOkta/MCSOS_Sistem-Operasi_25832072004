#!/usr/bin/env bash
set -Eeuo pipefail

KERNEL="${1:-build/kernel.elf}"
fail() { echo "FAIL: $*" >&2; exit 1; }
pass() { echo "PASS: $*"; }

test -f "$KERNEL" || fail "kernel ELF tidak ditemukan: $KERNEL"

readelf -h "$KERNEL" | tee build/m3_audit_readelf_header.txt
readelf -l "$KERNEL" | tee build/m3_audit_readelf_programs.txt
nm -n "$KERNEL" | tee build/m3_audit_symbols.txt >/dev/null
objdump -d -Mintel "$KERNEL" > build/m3_audit_disasm.txt

grep -q 'ELF64' build/m3_audit_readelf_header.txt || fail "bukan ELF64"
grep -q 'Advanced Micro Devices X86-64' build/m3_audit_readelf_header.txt || fail "machine bukan x86-64"
grep -q 'kmain' build/m3_audit_symbols.txt || fail "simbol kmain tidak ditemukan"
grep -q 'kernel_panic_at' build/m3_audit_symbols.txt || fail "simbol kernel_panic_at tidak ditemukan"
if nm -u "$KERNEL" | grep .; then
  fail "masih ada undefined symbol"
fi
if readelf -d "$KERNEL" >/tmp/m3_dynamic.$$ 2>&1 && grep -q 'Dynamic section' /tmp/m3_dynamic.$$; then
  rm -f /tmp/m3_dynamic.$$
  fail "kernel memiliki dynamic section; harus static freestanding"
fi
rm -f /tmp/m3_dynamic.$$

grep -q 'cli' build/m3_audit_disasm.txt || fail "instruksi cli tidak terlihat dalam disassembly"
grep -q 'hlt' build/m3_audit_disasm.txt || fail "instruksi hlt tidak terlihat dalam disassembly"
pass "audit ELF M3 selesai"
