#!/usr/bin/env bash
set -euo pipefail

printf '[M8] checking repository baseline...\n'
required=(
  include/mcsos/kmem.h
  kernel/mm/kmem.c
  tests/test_kmem.c
  Makefile
)
for f in "${required[@]}"; do
  if [[ ! -f "$f" ]]; then
    printf '[FAIL] missing %s\n' "$f" >&2
    exit 1
  fi
done

printf '[M8] checking toolchain...\n'
command -v clang >/dev/null
command -v nm >/dev/null
command -v objdump >/dev/null
command -v readelf >/dev/null
command -v make >/dev/null

printf '[M8] tool versions...\n'
clang --version | head -n 1
ld.lld --version 2>/dev/null | head -n 1 || true
make --version | head -n 1

printf '[M8] freestanding object check...\n'
mkdir -p build/m8
clang -std=c17 -Wall -Wextra -Werror -ffreestanding -fno-builtin \
  -Iinclude -c kernel/mm/kmem.c -o build/m8/kmem.freestanding.o
nm -u build/m8/kmem.freestanding.o | tee build/m8/nm_u.txt
if [[ -s build/m8/nm_u.txt ]]; then
  printf '[FAIL] unresolved symbol found in kmem.freestanding.o\n' >&2
  exit 1
fi
objdump -dr build/m8/kmem.freestanding.o > build/m8/kmem.objdump.txt
readelf -h build/m8/kmem.freestanding.o > build/m8/readelf_h.txt

printf '[M8] host unit test...\n'
clang -std=c17 -Wall -Wextra -Werror -Iinclude \
  tests/test_kmem.c kernel/mm/kmem.c -o build/m8/test_kmem
./build/m8/test_kmem | tee build/m8/test_kmem.log

grep -q 'PASS' build/m8/test_kmem.log
printf '[PASS] M8 preflight completed.\n'
