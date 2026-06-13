#!/usr/bin/env bash
set -euo pipefail

echo "[M7-PREFLIGHT] pemeriksaan lingkungan, M0-M6, dan artefak M7 (struktur kernel/)"

need_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "[FAIL] command tidak ditemukan: $1" >&2
    exit 1
  fi
  echo "[OK] $1 -> $(command -v "$1")"
}

need_file() {
  if [ ! -f "$1" ]; then
    echo "[FAIL] file wajib tidak ada: $1" >&2
    exit 1
  fi
  echo "[OK] file ada: $1"
}

need_cmd git
need_cmd make
need_cmd clang
need_cmd ld.lld
need_cmd readelf
need_cmd objdump
need_cmd nm
need_cmd qemu-system-x86_64

need_file kernel/include/pmm.h
need_file kernel/core/pmm.c
need_file kernel/include/vmm.h
need_file kernel/core/vmm.c
need_file tests/test_vmm_host.c
need_file Makefile

if ! grep -R "pmm_alloc_frame" kernel >/dev/null 2>&1; then
  echo "[FAIL] API pmm_alloc_frame dari M6 tidak ditemukan" >&2
  exit 1
fi
if ! grep -R "pmm_free_frame" kernel >/dev/null 2>&1; then
  echo "[FAIL] API pmm_free_frame dari M6 tidak ditemukan" >&2
  exit 1
fi

if ! grep -R "x86_64_trap_dispatch" kernel >/dev/null 2>&1; then
  echo "[WARN] dispatcher trap M4 belum ditemukan; page fault logging M7 harus diintegrasikan manual"
else
  echo "[OK] dispatcher trap M4 terdeteksi"
fi

if ! grep -R "timer" kernel >/dev/null 2>&1; then
  echo "[WARN] artefak timer M5 belum terdeteksi"
else
  echo "[OK] artefak timer M5 terdeteksi"
fi

echo "--- make clean ---"
make clean >/dev/null 2>&1 || true

echo "--- make check-m6 ---"
make check-m6

echo "--- make check-m7 ---"
make check-m7

echo "[PASS] M7 preflight selesai (struktur kernel/). Lanjutkan integrasi QEMU hanya setelah laporan M0-M6 lengkap."
