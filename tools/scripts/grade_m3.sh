#!/usr/bin/env bash
set -Eeuo pipefail

score=0
failures=0
check() {
  local points="$1"; shift
  local name="$1"; shift
  if "$@"; then
    echo "PASS[$points]: $name"
    score=$((score + points))
  else
    echo "FAIL[$points]: $name" >&2
    failures=$((failures + 1))
  fi
}

check 10 "preflight script valid" bash -n tools/scripts/m3_preflight.sh
check 10 "audit script valid" bash -n tools/scripts/m3_audit_elf.sh
check 20 "normal kernel build" make build
check 10 "panic-test kernel build" make panic
check 20 "ELF/disassembly audit" make audit
check 10 "panic symbol exists" grep -q kernel_panic_at build/kernel.syms.txt
check 10 "no undefined symbols" bash -c '! nm -u build/kernel.elf | grep .'
check 10 "evidence collection" tools/scripts/m3_collect_evidence.sh evidence/M3

echo "SCORE=$score/100"
if [ "$failures" -ne 0 ]; then
  exit 1
fi
