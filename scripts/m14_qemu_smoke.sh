#!/usr/bin/env bash
set -euo pipefail

ISO_PATH="${1:-build/mcsos.iso}"
LOG_PATH="${2:-artifacts/m14/qemu_m14.log}"
mkdir -p "$(dirname "$LOG_PATH")"

if ! command -v qemu-system-x86_64 >/dev/null 2>&1; then
  echo "[FAIL] qemu-system-x86_64 tidak ditemukan" >&2
  exit 1
fi
if [ ! -f "$ISO_PATH" ]; then
  echo "[FAIL] ISO tidak ditemukan: $ISO_PATH" >&2
  echo "Jalankan tools/scripts/make_iso.sh terlebih dahulu." >&2
  exit 1
fi

timeout 20s qemu-system-x86_64 \
  -machine q35 \
  -m 256M \
  -no-reboot \
  -no-shutdown \
  -serial file:"$LOG_PATH" \
  -display none \
  -cdrom "$ISO_PATH" || true

if [ ! -s "$LOG_PATH" ]; then
  echo "[FAIL] serial log kosong: $LOG_PATH" >&2
  exit 1
fi

if grep -qF '[M14] block: ram0 registered' "$LOG_PATH"; then
  echo "[OK] M14 block layer terdeteksi boot di QEMU: $LOG_PATH"
else
  echo "[FAIL] marker M14 block tidak ditemukan di $LOG_PATH" >&2
  exit 1
fi

if grep -qF 'KERNEL PANIC' "$LOG_PATH"; then
  echo "[FAIL] kernel panic terdeteksi selama boot M14" >&2
  exit 1
fi
