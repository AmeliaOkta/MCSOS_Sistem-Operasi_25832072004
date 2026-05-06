#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUT="$ROOT/build/meta"
mkdir -p "$OUT"

{
    echo "[qemu-version]"
    qemu-system-x86_64 --version
    echo
    echo "[qemu-machine-help-q35]"
    qemu-system-x86_64 -machine help | grep -E "q35|pc-q35" || true
    echo
    echo "[qemu-accel-help]"
    qemu-system-x86_64 -accel help || true
    echo
    echo "[ovmf-candidates]"
    for path in /usr/share/OVMF/OVMF_CODE.fd /usr/share/OVMF/OVMF_CODE_4M.fd \
    /usr/share/ovmf/OVMF.fd /usr/share/qemu/OVMF.fd; do
        if [ -r "$path" ]; then
            echo "$path"
        fi
    done
} | tee "$OUT/qemu-capabilities.txt"

if ! grep -q "q35" "$OUT/qemu-capabilities.txt"; then
    echo "ERROR: q35 machine not found in QEMU machine list" >&2
    exit 1
fi

if ! grep -q "OVMF" "$OUT/qemu-capabilities.txt" && ! grep -q "ovmf" "$OUT/qemu-capabilities.txt"; then
    echo "ERROR: OVMF firmware candidate not found" >&2
    exit 1
fi

echo "OK: QEMU and OVMF probe complete"
