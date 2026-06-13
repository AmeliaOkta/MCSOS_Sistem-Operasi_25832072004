#!/usr/bin/env bash
set -euo pipefail

mkdir -p build
: "${CC:=clang}"
: "${HOSTCC:=clang}"

${CC} -std=c17 -Wall -Wextra -Werror \
  -ffreestanding -fno-builtin -fno-stack-protector -mno-red-zone \
  -Ikernel/include -Ikernel/arch/x86_64/include -c kernel/core/pmm.c -o build/pmm.o

${HOSTCC} -std=c17 -Wall -Wextra -Werror \
  -Ikernel/include -Ikernel/arch/x86_64/include kernel/core/pmm.c tests/test_pmm_host.c -o build/test_pmm_host

./build/test_pmm_host
nm -u build/pmm.o | tee build/pmm.undefined.txt
objdump -dr build/pmm.o > build/pmm.objdump.txt

if grep -q . build/pmm.undefined.txt; then
  echo "[FAIL] pmm.o masih memiliki unresolved symbol" >&2
  exit 1
fi

echo "[PASS] M6 static check selesai"
