#!/usr/bin/env bash
set -e
mkdir -p logs
make run-qemu-smoke 2>&1 | tee logs/m10_qemu_run.log
grep -E "\[M10\]" build/qemu-serial.log
echo "[M10] qemu smoke: done"
