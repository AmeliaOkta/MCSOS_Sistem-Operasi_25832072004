#!/usr/bin/env bash
set -euo pipefail

ROOT="$(git rev-parse --show-toplevel 2>/dev/null || pwd)"
cd "$ROOT"
mkdir -p build/meta
REPORT="build/meta/m2-preflight.txt"
: > "$REPORT"

log() {
  printf '%s\n' "$*" | tee -a "$REPORT"
}

fail() {
  log "ERROR: $*"
  exit 1
}

need_cmd() {
  if command -v "$1" >/dev/null 2>&1; then
    log "OK command: $1 -> $(command -v "$1")"
  else
    fail "command tidak ditemukan: $1"
  fi
}

log "== M2 preflight MCSOS 260502 =="
log "root=$ROOT"
log "date_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"

case "$ROOT" in
  /mnt/c/*|/mnt/d/*|/mnt/e/*)
    fail "repository berada di filesystem Windows. Pindahkan ke filesystem Linux WSL, misalnya ~/src/mcsos."
    ;;
  *)
    log "OK filesystem: repository bukan /mnt/c, /mnt/d, atau /mnt/e"
    ;;
esac

need_cmd git
need_cmd make
need_cmd clang
need_cmd ld.lld
need_cmd readelf
need_cmd objdump
need_cmd nm
need_cmd qemu-system-x86_64
need_cmd xorriso
need_cmd python3

for f in \
  docs/architecture/overview.md \
  docs/architecture/invariants.md \
  docs/security/threat_model.md \
  docs/testing/verification_matrix.md; do
  if [ -f "$f" ]; then
    log "OK M0 file: $f"
  else
    fail "artefak M0 belum ada: $f"
  fi
done

if [ -f build/meta/toolchain-versions.txt ]; then
  log "OK M1 metadata: build/meta/toolchain-versions.txt"
else
  log "WARN: build/meta/toolchain-versions.txt belum ada; menjalankan make meta jika tersedia"
  if make -n meta >/dev/null 2>&1; then
    make meta
  else
    fail "target make meta tidak tersedia dan metadata M1 belum ada"
  fi
fi

if [ -f build/proof/freestanding_probe.o ]; then
  readelf -hW build/proof/freestanding_probe.o > build/meta/m2-check-m1-object-readelf.txt
  grep -q 'Class:.*ELF64' build/meta/m2-check-m1-object-readelf.txt || fail "object M1 bukan ELF64"
  grep -q 'Machine:.*Advanced Micro Devices X86-64' build/meta/m2-check-m1-object-readelf.txt || fail "object M1 bukan x86_64"
  log "OK M1 proof object: ELF64 x86_64"
else
  log "WARN: build/proof/freestanding_probe.o tidak ditemukan. Pastikan M1 sudah dinilai atau jalankan ulang target proof M1."
fi

if find /usr/share -type f \( -name 'OVMF_CODE*.fd' -o -name 'OVMF_VARS*.fd' \) 2>/dev/null | grep -q OVMF; then
  find /usr/share -type f \( -name 'OVMF_CODE*.fd' -o -name 'OVMF_VARS*.fd' \) 2>/dev/null | sort | tee -a "$REPORT"
else
  fail "OVMF tidak ditemukan pada /usr/share. Pasang paket ovmf."
fi

log "OK: preflight M2 selesai"
