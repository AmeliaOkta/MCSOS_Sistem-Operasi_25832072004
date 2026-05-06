#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUT="$ROOT/build/meta"
mkdir -p "$OUT"

{
    echo "mcsos_milestone=M1"
    echo "date_utc=$(date -u +"%Y-%m-%dT%H:%M:%SZ")"
    echo "repo=$ROOT"
    echo "pwd=$(pwd)"
    echo "user=$(id -un)"
    echo "uname=$(uname -a)"
    echo "nproc=$(nproc)"
    echo "shell=$SHELL"
    echo "path=$PATH"
    echo
    echo "[os-release]"
    cat /etc/os-release || true
    echo
    echo "[tool-versions]"
    git --version || true
    make --version | head -n 1 || true
    cmake --version | head -n 1 || true
    ninja --version || true
    clang --version | head -n 1 || true
    ld.lld --version | head -n 1 || true
    llvm-objdump --version | head -n 1 || true
    gcc --version | head -n 1 || true
    readelf --version | head -n 1 || true
    objdump --version | head -n 1 || true
    nm --version | head -n 1 || true
    nasm -v || true
    qemu-system-x86_64 --version | head -n 1 || true
    gdb --version | head -n 1 || true
    python3 --version || true
    shellcheck --version | head -n 2 || true
    cppcheck --version || true
    clang-tidy --version | head -n 1 || true
} | tee "$OUT/toolchain-versions.txt"

{
    echo "[filesystem]"
    df -h "$ROOT"
    echo
    echo "[memory]"
    free -h || true
    echo
    echo "[cpu]"
    lscpu || true
} | tee "$OUT/host-readiness.txt"
