#!/usr/bin/env bash
set -Eeuo pipefail

fail() { echo "FAIL: $*" >&2; exit 1; }
warn() { echo "WARN: $*" >&2; }
pass() { echo "PASS: $*"; }
need_cmd() { command -v "$1" >/dev/null 2>&1 || fail "command tidak ditemukan: $1"; }

ROOT="$(git rev-parse --show-toplevel 2>/dev/null || pwd)"
cd "$ROOT"

echo "[M3 preflight] root=$ROOT"
case "$ROOT" in
  /mnt/c/*|/mnt/d/*|/mnt/e/*) warn "repository berada di filesystem Windows; pindahkan ke ~/mcsos untuk I/O build lebih stabil" ;;
  *) pass "repository berada di filesystem Linux/WSL" ;;
esac

need_cmd git
need_cmd clang
need_cmd ld.lld
need_cmd make
need_cmd readelf
need_cmd objdump
need_cmd nm

if command -v qemu-system-x86_64 >/dev/null 2>&1; then
  pass "QEMU tersedia: $(qemu-system-x86_64 --version | head -n 1)"
else
  warn "qemu-system-x86_64 belum tersedia; build/audit tetap bisa berjalan, run QEMU belum bisa"
fi

if git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  git status --short
else
  warn "direktori ini belum menjadi repository Git"
fi

for path in linker.ld kernel/core/serial.c kernel/lib/memory.c kernel/core/kmain.c; do
  test -e "$path" || fail "artefak M2 hilang: $path"
done

test -d kernel/arch/x86_64/include/mcsos/arch || fail "include arsitektur x86_64 belum ada"

echo "[M3 preflight] compiler=$(clang --version | head -n 1)"
echo "[M3 preflight] linker=$(ld.lld --version | head -n 1)"
pass "preflight M3 selesai"
