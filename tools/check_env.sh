#!/usr/bin/env bash
echo "[MO] Memeriksa Lingkungan Pengembangan..."
ROOT_DIR=$(pwd)
META_DIR="$ROOT_DIR/build/meta"
mkdir -p "$META_DIR"

check_tool() {
    if command -v "$1" >/dev/null 2>&1; then
        echo "[OK] $1 ditemukan di $(command -v $1)"
    else
        echo "[FAIL] $1 tidak ditemukan"
    fi
}

for t in git make clang ld.lld nasm qemu-system-x86_64 gdb python3; do
    check_tool "$t"
done

# Menulis Metadata
{
    echo "date_utc=$(date -u)"
    echo "uname=$(uname -a)"
    clang --version | head -n 1
    qemu-system-x86_64 --version | head -n 1
} > "$META_DIR/toolchain-versions.txt"
echo "[MO] Metadata berhasil ditulis ke build/meta/toolchain-versions.txt"
