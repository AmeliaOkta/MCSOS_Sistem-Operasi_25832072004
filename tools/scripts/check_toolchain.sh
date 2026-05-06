#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
STATUS=0

check_cmd() {
    local name="$1"
    if command -v "$name" >/dev/null 2>&1; then
        printf "OK: %-24s %s\n" "$name" "$(command -v "$name")"
    else
        printf "ERROR: missing command: %s\n" "$name" >&2
        STATUS=1
    fi
}

case "$ROOT" in
    /mnt/*)
        echo "ERROR: repository must not be under /mnt/*; current root=$ROOT" >&2
        STATUS=1
        ;;
    *)
        echo "OK: repository path is WSL Linux filesystem: $ROOT"
        ;;
esac

for cmd in git make cmake ninja clang ld.lld llvm-objdump gcc readelf \
objdump nm nasm qemu-system-x86_64 gdb python3 shellcheck cppcheck clang-tidy file; do
    check_cmd "$cmd"
done

OVMF_FOUND=0
for path in \
    /usr/share/OVMF/OVMF_CODE.fd \
    /usr/share/OVMF/OVMF_CODE_4M.fd \
    /usr/share/ovmf/OVMF.fd \
    /usr/share/qemu/OVMF.fd; do
    if [ -r "$path" ]; then
        echo "OK: OVMF firmware found: $path"
        OVMF_FOUND=1
    fi
done

if [ "$OVMF_FOUND" -eq 0 ]; then
    echo "ERROR: OVMF firmware not found. Install package: ovmf" >&2
    STATUS=1
fi

exit "$STATUS"
