#!/usr/bin/env bash
set -Eeuo pipefail

ISO="${1:-build/mcsos.iso}"

test -f "$ISO" || { echo "FAIL: ISO tidak ditemukan: $ISO" >&2; exit 1; }

exec qemu-system-x86_64 \
    -m 256M \
    -cdrom "$ISO" \
    -serial stdio \
    -s -S
