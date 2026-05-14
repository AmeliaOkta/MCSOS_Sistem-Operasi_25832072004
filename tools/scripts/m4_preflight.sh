#!/usr/bin/env bash
set -euo pipefail

fail() { echo "[M4][FAIL] $*" >&2; exit 1; }
pass() { echo "[M4][PASS] $*"; }
warn() { echo "[M4][WARN] $*" >&2; }

[[ -d .git ]] || fail "Jalankan dari root repository Git MCSOS."
[[ -f linker.ld ]] || fail "linker.ld belum ada. Selesaikan M2/M3 terlebih dahulu."
[[ -f Makefile ]] || fail "Makefile belum ada. Selesaikan M1/M2/M3 terlebih dahulu."
[[ -f kernel/include/mcsos/kernel/log.h ]] || fail "Header log M3 tidak ditemukan."
[[ -f kernel/include/mcsos/kernel/panic.h ]] || fail "Header panic M3 tidak ditemukan."
[[ -f kernel/core/panic.c ]] || fail "panic.c M3 tidak ditemukan."
[[ -f kernel/core/serial.c ]] || fail "serial.c M3 tidak ditemukan."

command -v clang >/dev/null || fail "clang tidak ditemukan. Jalankan setup toolchain M1."
command -v ld.lld >/dev/null || fail "ld.lld tidak ditemukan. Jalankan setup toolchain M1."
command -v readelf >/dev/null || fail "readelf tidak ditemukan. Instal binutils."
command -v objdump >/dev/null || fail "objdump tidak ditemukan. Instal binutils."
command -v nm >/dev/null || fail "nm tidak ditemukan. Instal binutils."

if ! command -v qemu-system-x86_64 >/dev/null; then
  warn "qemu-system-x86_64 tidak ditemukan. Build dapat diuji, tetapi smoke test QEMU belum dapat dijalankan."
else
  pass "QEMU tersedia: $(qemu-system-x86_64 --version | head -n 1)"
fi

pass "clang: $(clang --version | head -n 1)"
pass "ld.lld: $(ld.lld --version | head -n 1)"
pass "readelf: $(readelf --version | head -n 1)"
pass "M0/M1/M2/M3 readiness minimum untuk M4 terpenuhi."
