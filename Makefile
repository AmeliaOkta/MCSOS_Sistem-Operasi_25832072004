.RECIPEPREFIX := >
SHELL := /usr/bin/env bash

# ── Directories ────────────────────────────────────────────────────────────
BUILD_DIR := build

# ── Output artifacts ───────────────────────────────────────────────────────
KERNEL       := $(BUILD_DIR)/kernel.elf
BP_KERNEL    := $(BUILD_DIR)/kernel.breakpoint.elf
PANIC_KERNEL := $(BUILD_DIR)/kernel.panic.elf

MAP       := $(BUILD_DIR)/kernel.map
BP_MAP    := $(BUILD_DIR)/kernel.breakpoint.map
PANIC_MAP := $(BUILD_DIR)/kernel.panic.map

DISASM := $(BUILD_DIR)/kernel.disasm.txt
SYMS   := $(BUILD_DIR)/kernel.syms.txt

# ── Toolchain ──────────────────────────────────────────────────────────────
CC      := clang
HOSTCC  ?= cc
LD      := ld.lld
OBJDUMP := objdump
READELF := readelf
NM      := nm

# ── Compiler flags ─────────────────────────────────────────────────────────
COMMON_CFLAGS := \
    --target=x86_64-unknown-none-elf \
    -std=c17 \
    -ffreestanding \
    -fno-builtin \
    -fno-stack-protector \
    -fno-stack-check \
    -fno-pic \
    -fno-pie \
    -fno-lto \
    -m64 \
    -march=x86-64 \
    -mabi=sysv \
    -mno-red-zone \
    -mno-mmx \
    -mno-sse \
    -mno-sse2 \
    -mcmodel=kernel \
    -Wall \
    -Wextra \
    -Werror \
    -Ikernel/arch/x86_64/include \
    -Ikernel/include \
    -Ilimine

COMMON_ASFLAGS := \
    --target=x86_64-unknown-none-elf \
    -ffreestanding \
    -fno-pic \
    -fno-pie \
    -m64 \
    -mno-red-zone \
    -Wall \
    -Wextra \
    -Werror \
    -Ikernel/arch/x86_64/include \
    -Ikernel/include \
    -Ilimine

CFLAGS     := $(COMMON_CFLAGS)
ASFLAGS    := $(COMMON_ASFLAGS)
BP_CFLAGS  := $(COMMON_CFLAGS) -DMCSOS_M4_TRIGGER_BREAKPOINT=1
PANIC_CFLAGS := $(COMMON_CFLAGS) -DMCSOS_M4_TRIGGER_PANIC=1

LDFLAGS := \
    -nostdlib \
    -static \
    -z max-page-size=0x1000 \
    -T linker.ld

# ── M6 host test flags ─────────────────────────────────────────────────────
M6_HOST_CFLAGS := \
    -std=c17 \
    -Wall \
    -Wextra \
    -Werror \
    -Ikernel/include \
    -Ikernel/arch/x86_64/include

# ── Sources ────────────────────────────────────────────────────────────────
SRC_C := $(shell find kernel -name '*.c' | LC_ALL=C sort)
SRC_S := $(shell find kernel -name '*.S' | LC_ALL=C sort)

OBJ := \
    $(patsubst %.c,$(BUILD_DIR)/normal/%.o,$(SRC_C)) \
    $(patsubst %.S,$(BUILD_DIR)/normal/%.o,$(SRC_S))

BP_OBJ := \
    $(patsubst %.c,$(BUILD_DIR)/breakpoint/%.o,$(SRC_C)) \
    $(patsubst %.S,$(BUILD_DIR)/breakpoint/%.o,$(SRC_S))

PANIC_OBJ := \
    $(patsubst %.c,$(BUILD_DIR)/panic/%.o,$(SRC_C)) \
    $(patsubst %.S,$(BUILD_DIR)/panic/%.o,$(SRC_S))

# ── Phony targets ──────────────────────────────────────────────────────────
.PHONY: all build breakpoint panic inspect audit \
        check-m6 make-iso run-qemu-smoke \
        clean distclean

# ── Default target ─────────────────────────────────────────────────────────
all: build inspect

# ══════════════════════════════════════════════════════════════════════════
# M0–M5: Kernel build targets
# ══════════════════════════════════════════════════════════════════════════

build: $(KERNEL)
breakpoint: $(BP_KERNEL)
panic: $(PANIC_KERNEL)

# ── Compile rules: normal ──────────────────────────────────────────────────
$(BUILD_DIR)/normal/%.o: %.c
>mkdir -p $(dir $@)
>$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/normal/%.o: %.S
>mkdir -p $(dir $@)
>$(CC) $(ASFLAGS) -c $< -o $@

# ── Compile rules: breakpoint ──────────────────────────────────────────────
$(BUILD_DIR)/breakpoint/%.o: %.c
>mkdir -p $(dir $@)
>$(CC) $(BP_CFLAGS) -c $< -o $@

$(BUILD_DIR)/breakpoint/%.o: %.S
>mkdir -p $(dir $@)
>$(CC) $(ASFLAGS) -c $< -o $@

# ── Compile rules: panic ───────────────────────────────────────────────────
$(BUILD_DIR)/panic/%.o: %.c
>mkdir -p $(dir $@)
>$(CC) $(PANIC_CFLAGS) -c $< -o $@

$(BUILD_DIR)/panic/%.o: %.S
>mkdir -p $(dir $@)
>$(CC) $(ASFLAGS) -c $< -o $@

# ── Link rules ─────────────────────────────────────────────────────────────
$(KERNEL): $(OBJ) linker.ld
>mkdir -p $(BUILD_DIR)
>$(LD) $(LDFLAGS) -Map=$(MAP) -o $@ $(OBJ)

$(BP_KERNEL): $(BP_OBJ) linker.ld
>mkdir -p $(BUILD_DIR)
>$(LD) $(LDFLAGS) -Map=$(BP_MAP) -o $@ $(BP_OBJ)

$(PANIC_KERNEL): $(PANIC_OBJ) linker.ld
>mkdir -p $(BUILD_DIR)
>$(LD) $(LDFLAGS) -Map=$(PANIC_MAP) -o $@ $(PANIC_OBJ)

# ── Inspect ────────────────────────────────────────────────────────────────
inspect: $(KERNEL)
>$(READELF) -h $(KERNEL) > $(BUILD_DIR)/kernel.readelf.header.txt
>$(READELF) -l $(KERNEL) > $(BUILD_DIR)/kernel.readelf.programs.txt
>$(NM) -n $(KERNEL) > $(SYMS)
>$(OBJDUMP) -d -Mintel $(KERNEL) > $(DISASM)
>grep -q 'ELF64' $(BUILD_DIR)/kernel.readelf.header.txt
>grep -q 'Machine:[[:space:]]*Advanced Micro Devices X86-64' \
>    $(BUILD_DIR)/kernel.readelf.header.txt
>grep -q 'kmain'                  $(SYMS)
>grep -q 'x86_64_idt_init'        $(SYMS)
>grep -q 'x86_64_trap_dispatch'   $(SYMS)
>grep -q 'iretq'                  $(DISASM)
>grep -q 'lidt'                   $(DISASM)

# ── Audit ──────────────────────────────────────────────────────────────────
audit: inspect breakpoint panic
>! $(NM) -u $(KERNEL)    | grep .
>! $(NM) -u $(BP_KERNEL) | grep .
>! $(NM) -u $(PANIC_KERNEL) | grep .
>grep -q 'isr_stub_14'           $(SYMS)
>grep -q 'x86_64_exception_stubs' $(SYMS)
>$(READELF) -S $(KERNEL) | grep -q '.text'
>$(READELF) -S $(KERNEL) | grep -q '.rodata'

# ══════════════════════════════════════════════════════════════════════════
# M6: PMM targets
# ══════════════════════════════════════════════════════════════════════════

# ── Freestanding PMM object (kernel target) ────────────────────────────────
build/pmm.o: kernel/core/pmm.c \
             kernel/include/pmm.h \
             kernel/include/types.h
>mkdir -p build
>$(CC) $(COMMON_CFLAGS) -c kernel/core/pmm.c -o build/pmm.o

# ── Host unit test binary ──────────────────────────────────────────────────
build/test_pmm_host: kernel/core/pmm.c \
                     tests/test_pmm_host.c \
                     kernel/include/pmm.h \
                     kernel/include/types.h
>mkdir -p build
>$(HOSTCC) $(M6_HOST_CFLAGS) \
>    kernel/core/pmm.c \
>    tests/test_pmm_host.c \
>    -o build/test_pmm_host

# ── M6 check: unit test + freestanding audit ───────────────────────────────
check-m6: build/pmm.o build/test_pmm_host
>./build/test_pmm_host
>$(NM) -u build/pmm.o | tee build/pmm.undefined.txt
>test ! -s build/pmm.undefined.txt
>$(OBJDUMP) -dr build/pmm.o > build/pmm.objdump.txt
>echo "[PASS] M6 check selesai"

# ── ISO build ──────────────────────────────────────────────────────────────
make-iso:
>bash tools/scripts/make_iso.sh

# ── QEMU smoke test ────────────────────────────────────────────────────────
run-qemu-smoke: make-iso
>bash tools/scripts/run_qemu.sh 2>&1 | tee build/m6_qemu.log || true
>grep -E "\[MCSOS|pmm|panic|fault" build/qemu-serial.log || true

# ══════════════════════════════════════════════════════════════════════════
# Clean
# ══════════════════════════════════════════════════════════════════════════

# ══════════════════════════════════════════════════════════════════════════
# M7: VMM targets
# ══════════════════════════════════════════════════════════════════════════

M7_HOST_CFLAGS := -std=c17 -Wall -Wextra -Werror -Ikernel/include -Ikernel/arch/x86_64/include -DMCSOS_HOST_TEST

# ── Freestanding VMM object (kernel target) ────────────────────────────────
build/vmm.o: kernel/core/vmm.c \
             kernel/include/vmm.h \
             kernel/include/types.h
>mkdir -p build
>$(CC) $(COMMON_CFLAGS) -c kernel/core/vmm.c -o build/vmm.o

# ── Host unit test binary ──────────────────────────────────────────────────
build/test_vmm_host: kernel/core/vmm.c \
                     tests/test_vmm_host.c \
                     kernel/include/vmm.h \
                     kernel/include/types.h
>mkdir -p build
>$(HOSTCC) $(M7_HOST_CFLAGS) \
>    kernel/core/vmm.c \
>    tests/test_vmm_host.c \
>    -o build/test_vmm_host

# ── M7 check: unit test + freestanding audit ───────────────────────────────
check-m7: build/vmm.o build/test_vmm_host
>./build/test_vmm_host
>$(NM) -u build/vmm.o | tee build/vmm.undefined.txt
>test ! -s build/vmm.undefined.txt
>$(OBJDUMP) -dr build/vmm.o > build/vmm.objdump.txt
>grep -q "invlpg" build/vmm.objdump.txt
>grep -q "cr3" build/vmm.objdump.txt
>echo "[PASS] M7 check selesai"

clean:
>rm -rf $(BUILD_DIR)

distclean: clean
>rm -rf iso_root limine evidence

run-qemu-gdb: make-iso
>bash tools/scripts/run_qemu_debug.sh 2>&1 | tee build/m6_qemu_gdb.log || true
