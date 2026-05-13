#!/usr/bin/env bash
set -euo pipefail

ISO="build/mcsos.iso"
LOG="build/qemu-serial.log"
OVMF_CODE=""
OVMF_VARS_TEMPLATE=""
OVMF_VARS="build/OVMF_VARS.fd"

find_first() {
  for f in "$@"; do
    if [ -f "$f" ]; then
      printf '%s\n' "$f"
      return 0
    fi
  done
  return 1
}

OVMF_CODE="$(find_first \
  /usr/share/OVMF/OVMF_CODE_4M.fd \
  /usr/share/OVMF/OVMF_CODE.fd \
  /usr/share/edk2/ovmf/OVMF_CODE.fd \
  /usr/share/qemu/OVMF_CODE.fd || true)"

OVMF_VARS_TEMPLATE="$(find_first \
  /usr/share/OVMF/OVMF_VARS_4M.fd \
  /usr/share/OVMF/OVMF_VARS.fd \
  /usr/share/edk2/ovmf/OVMF_VARS.fd \
  /usr/share/qemu/OVMF_VARS.fd || true)"

if [ ! -f "$ISO" ]; then
  echo "ERROR: $ISO tidak ditemukan. Jalankan make image." >&2
  exit 1
fi

if [ -z "$OVMF_CODE" ]; then
  echo "ERROR: OVMF_CODE tidak ditemukan. Pasang paket ovmf." >&2
  exit 1
fi

rm -f "$LOG"
mkdir -p build

QEMU_ARGS=(
  -machine q35
  -cpu qemu64
  -m 512M
  -serial "file:$LOG"
  -display none
  -monitor none
  -no-reboot
  -no-shutdown
  -drive "if=pflash,format=raw,readonly=on,file=$OVMF_CODE"
  -cdrom "$ISO"
)

if [ -n "$OVMF_VARS_TEMPLATE" ]; then
  cp "$OVMF_VARS_TEMPLATE" "$OVMF_VARS"
  QEMU_ARGS+=( -drive "if=pflash,format=raw,file=$OVMF_VARS" )
fi

timeout 10s qemu-system-x86_64 "${QEMU_ARGS[@]}" || status=$?
status="${status:-0}"
if [ "$status" != "0" ] && [ "$status" != "124" ]; then
  echo "ERROR: QEMU keluar dengan status $status" >&2
  exit "$status"
fi

if [ ! -s "$LOG" ]; then
  echo "ERROR: serial log kosong: $LOG" >&2
  exit 1
fi

grep -q 'MCSOS 260502 M2 boot path entered' "$LOG"
grep -q '\[M2\] early serial online' "$LOG"
grep -q '\[M2\] kernel reached controlled halt loop' "$LOG"

echo "OK: QEMU serial log valid: $LOG"
