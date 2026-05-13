#!/usr/bin/env bash
set -euo pipefail

ISO="build/mcsos.iso"
LOG="build/qemu-debug-serial.log"
OVMF_CODE=""

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

test -f "$ISO"
test -n "$OVMF_CODE"

rm -f "$LOG"
qemu-system-x86_64 \
  -machine q35 \
  -cpu qemu64 \
  -m 512M \
  -serial "file:$LOG" \
  -display none \
  -monitor stdio \
  -no-reboot \
  -no-shutdown \
  -drive "if=pflash,format=raw,readonly=on,file=$OVMF_CODE" \
  -cdrom "$ISO" \
  -s -S
