#!/usr/bin/env bash
set -euo pipefail
mkdir -p artifacts/m14
LOG="artifacts/m14/preflight.log"
: > "$LOG"

require_file() {
  local path="$1"
  if [[ ! -e "$path" ]]; then
    echo "MISSING: $path" | tee -a "$LOG"
    return 1
  fi
  echo "OK: $path" | tee -a "$LOG"
}

require_cmd() {
  local cmd="$1"
  if ! command -v "$cmd" >/dev/null 2>&1; then
    echo "MISSING_CMD: $cmd" | tee -a "$LOG"
    return 1
  fi
  echo "OK_CMD: $cmd=$($cmd --version 2>/dev/null | head -n 1 || true)" | tee -a "$LOG"
}

require_cmd clang
require_cmd ld
require_cmd nm
require_cmd readelf
require_cmd objdump
require_cmd sha256sum
require_cmd make
require_cmd qemu-system-x86_64

for d in include kernel tests scripts; do
  [[ -d "$d" ]] && echo "OK_DIR: $d" | tee -a "$LOG" || echo "WARN_DIR_MISSING: $d" | tee -a "$LOG"
done

for f in OS_panduan_M0.md OS_panduan_M1.md OS_panduan_M2.md OS_panduan_M3.md OS_panduan_M4.md OS_panduan_M5.md OS_panduan_M6.md OS_panduan_M7.md OS_panduan_M8.md OS_panduan_M9.md OS_panduan_M10.md OS_panduan_M11.md OS_panduan_M12.md OS_panduan_M13.md; do
  [[ -e "$f" ]] && echo "OK_DOC: $f" | tee -a "$LOG" || echo "WARN_DOC_NOT_FOUND_IN_REPO: $f" | tee -a "$LOG"
done

git status --short | tee artifacts/m14/git_status_before_m14.txt
if [[ -s artifacts/m14/git_status_before_m14.txt ]]; then
  echo "WARN: working tree tidak bersih; commit atau stash perubahan sebelum final grading" | tee -a "$LOG"
fi

echo "M14_PREFLIGHT_DONE" | tee -a "$LOG"
