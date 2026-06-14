#!/usr/bin/env bash
set -e
echo "[M10] preflight: toolchain"
clang --version || true
nm --version | head -1
readelf --version | head -1
objdump --version | head -1
qemu-system-x86_64 --version || true
echo "[M10] preflight: done"
