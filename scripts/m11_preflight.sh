#!/usr/bin/env bash
set -euo pipefail

echo "[M11] Preflight lingkungan dan artefak M0-M10"
for tool in git make clang nm readelf objdump sha256sum; do
  if ! command -v "$tool" >/dev/null 2>&1; then
    echo "[FAIL] tool tidak ditemukan: $tool" >&2
    exit 1
  fi
  echo "[OK] $tool -> $(command -v "$tool")"
done

clang --version | sed -n '1,3p'
make --version | sed -n '1p'

required_dirs=(kernel arch include scripts tests)
for d in "${required_dirs[@]}"; do
  if [ ! -d "$d" ]; then
    echo "[WARN] direktori $d belum ada; sesuaikan dengan struktur repository MCSOS Anda"
  else
    echo "[OK] direktori $d tersedia"
  fi
done

required_markers=(
  "kernel_main"
  "panic"
  "idt"
  "pmm"
  "vmm"
  "kmalloc"
  "sched"
  "syscall"
)
for m in "${required_markers[@]}"; do
  if grep -R "${m}" -n kernel arch include 2>/dev/null | head -n 1 >/dev/null; then
    echo "[OK] marker ditemukan: $m"
  else
    echo "[WARN] marker belum ditemukan: $m"
  fi
done

if git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  echo "[OK] commit: $(git rev-parse --short HEAD)"
  git status --short
else
  echo "[WARN] direktori ini belum menjadi repository Git"
fi

