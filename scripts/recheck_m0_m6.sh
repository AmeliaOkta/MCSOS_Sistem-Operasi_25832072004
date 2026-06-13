#!/usr/bin/env bash
set -uo pipefail

PASS=0; FAIL=0; WARN=0
ok()   { echo "[OK]   $1"; PASS=$((PASS+1)); }
fail() { echo "[FAIL] $1"; FAIL=$((FAIL+1)); }
warn() { echo "[WARN] $1"; WARN=$((WARN+1)); }

echo "================ M0: ENVIRONMENT & REPO ================"
command -v git >/dev/null 2>&1 && ok "git -> $(git --version)" || fail "git tidak ditemukan"
if [ -d .git ]; then
  ok "repo git terdeteksi"
  echo "--- git status ---"; git status --short
  echo "--- 15 commit terakhir ---"; git log --oneline -15
else
  fail "bukan root repo git"
fi
echo "--- struktur top-level ---"; ls -la

echo; echo "================ M1: TOOLCHAIN ================"
for c in clang ld.lld readelf objdump nm make qemu-system-x86_64 gdb; do
  command -v "$c" >/dev/null 2>&1 && ok "$c -> $(command -v "$c")" || fail "$c tidak ditemukan"
done

echo; echo "================ M2: BOOT IMAGE / KERNEL ELF ================"
grep -qE "^(iso|all):" Makefile 2>/dev/null && ok "Makefile punya target iso/all" || warn "target iso/all tidak terdeteksi"
ls -la build/ 2>/dev/null || echo "(build/ belum ada)"

echo; echo "================ M3: SERIAL / PANIC / HALT ================"
grep -RIlq "serial_init\|serial_write" include src 2>/dev/null && ok "serial_init/serial_write ditemukan" || fail "serial_init/serial_write TIDAK ditemukan"
grep -RIlq "panic" include src 2>/dev/null && ok "fungsi panic ditemukan" || fail "panic TIDAK ditemukan"

echo; echo "================ M4: IDT / TRAP DISPATCH ================"
if grep -RIlq "x86_64_trap_dispatch" include src 2>/dev/null; then
  ok "x86_64_trap_dispatch ditemukan"
  grep -RIn "x86_64_trap_dispatch" include src
else
  fail "x86_64_trap_dispatch TIDAK ditemukan"
fi
grep -RIlq "lidt\|idt_init" include src 2>/dev/null && ok "idt_init/lidt ditemukan" || warn "idt_init/lidt tidak terdeteksi dengan nama itu"

echo; echo "================ M5: TIMER / IRQ ================"
grep -RIlq "timer\|pit\|pic_send_eoi\|irq0" include src 2>/dev/null && ok "artefak timer/PIC/IRQ ditemukan" || warn "artefak timer M5 tidak terdeteksi"

echo; echo "================ M6: PMM ================"
for f in include/pmm.h src/pmm.c; do
  [ -f "$f" ] && ok "$f ada" || fail "$f TIDAK ada"
done
grep -RIlq "pmm_alloc_frame" include src 2>/dev/null && ok "pmm_alloc_frame ditemukan" || fail "pmm_alloc_frame TIDAK ditemukan"
grep -RIlq "pmm_free_frame" include src 2>/dev/null && ok "pmm_free_frame ditemukan" || fail "pmm_free_frame TIDAK ditemukan"
[ -f tests/test_pmm_host.c ] && ok "tests/test_pmm_host.c ada" || warn "tests/test_pmm_host.c tidak ditemukan (nama mungkin beda)"

echo; echo "================ BUILD CHECK ================"
if [ -f Makefile ]; then
  make clean >/dev/null 2>&1 || true
  if make check 2>&1 | tee build/m0_m6_make_check.log; then
    ok "make check sukses (lihat build/m0_m6_make_check.log)"
  else
    fail "make check gagal (lihat build/m0_m6_make_check.log)"
  fi
else
  fail "Makefile tidak ada di root"
fi

echo; echo "================ SUMMARY ================"
echo "PASS=$PASS  FAIL=$FAIL  WARN=$WARN"
