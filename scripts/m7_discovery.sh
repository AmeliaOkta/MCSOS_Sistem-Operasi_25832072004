#!/usr/bin/env bash
set -uo pipefail

echo "================ BUILD: check-m6 ================"
make clean >/dev/null 2>&1 || true
make check-m6 2>&1 | tee build/check_m6.log

echo; echo "================ BUILD: full kernel (make build) ================"
make build 2>&1 | tee build/build.log

echo; echo "================ types.h ================"
cat kernel/include/types.h

echo; echo "================ pmm.h ================"
cat kernel/include/pmm.h

echo; echo "================ Makefile (lines 1-40, sekitar variabel) ================"
sed -n '1,40p' Makefile

echo; echo "================ Makefile (lines 180-240, sekitar check-m6/make-iso) ================"
sed -n '180,240p' Makefile

echo; echo "================ idt.h ================"
cat kernel/arch/x86_64/include/mcsos/arch/idt.h

echo; echo "================ trap.c ================"
cat kernel/core/trap.c

echo; echo "================ HHDM / Limine search ================"
grep -RIn "hhdm\|HHDM\|limine_request\|LIMINE" kernel configs include src 2>/dev/null | head -40

echo; echo "================ test_pmm_host.c ================"
cat tests/test_pmm_host.c
