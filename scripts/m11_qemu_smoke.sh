#!/usr/bin/env bash
set -euo pipefail

ISO_PATH="${1:-build/mcsos.iso}"
LOG_PATH="${2:-build/m11_qemu_serial.log}"
mkdir -p "$(dirname "$LOG_PATH")"

if ! command -v qemu-system-x86_64 >/dev/null 2>&1; then
  echo "[FAIL] qemu-system-x86_64 tidak ditemukan" >&2
  exit 1
fi
if [ ! -f "$ISO_PATH" ]; then
  echo "[FAIL] ISO tidak ditemukan: $ISO_PATH" >&2
  echo "Jalankan target build ISO dari M2-M10 terlebih dahulu." >&2
  exit 1
fi

timeout 20s qemu-system-x86_64 \
  -M q35 \
  -m 256M \
  -no-reboot \
  -no-shutdown \
  -serial file:"$LOG_PATH" \
  -display none \
  -cdrom "$ISO_PATH" || true

if grep -E "M11|ELF|user|loader|panic" "$LOG_PATH" >/dev/null 2>&1; then
  echo "[OK] log M11 terdeteksi di $LOG_PATH"
else
  echo "[WARN] marker M11 belum terlihat. Periksa integrasi kernel_main dan jalur serial."
fi
