#!/usr/bin/env bash
set -euo pipefail
mkdir -p artifacts/m15
{
  echo "== git =="
  git status --short || true
  git rev-parse --short HEAD || true
  echo "== toolchain =="
  clang --version | head -n 1
  ld --version | head -n 1
  nm --version | head -n 1
  readelf --version | head -n 1
  objdump --version | head -n 1
  make --version | head -n 1
  echo "== prior artifacts =="
  for d in m0 m1 m2 m3 m4 m5 m6 m7 m8 m9 m10 m11 m12 m13 m14; do
    if [ -d "artifacts/$d" ]; then
      echo "artifacts/$d: present"
    else
      echo "artifacts/$d: missing"
    fi
  done
} | tee artifacts/m15/preflight.txt
