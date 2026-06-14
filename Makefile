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
    -Ilimine \
    -Iinclude

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
    -Ilimine \
    -Iinclude

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
        check-m10 m10-host-test m10-freestanding m10-audit m10-clean \
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

# ══════════════════════════════════════════════════════════════════════════

# ══════════════════════════════════════════════════════════════════════════
# M8: Kernel Heap / kmem targets
# ══════════════════════════════════════════════════════════════════════════

M8_CFLAGS_KERNEL := \
    -std=c17 -Wall -Wextra -Werror \
    -ffreestanding -fno-builtin -fno-stack-protector \
    -mno-red-zone \
    -Iinclude

M8_CFLAGS_HOST := \
    -std=c17 -Wall -Wextra -Werror \
    -Iinclude

.PHONY: m8-clean m8-kmem-freestanding m8-kmem-host-test m8-audit m8-all

m8-clean:
> $(RM) -r build/m8

build/m8:
> mkdir -p build/m8

m8-kmem-freestanding: | build/m8
> $(CC) $(M8_CFLAGS_KERNEL) -c kernel/mm/kmem.c -o build/m8/kmem.freestanding.o

m8-kmem-host-test: | build/m8
> $(HOSTCC) $(M8_CFLAGS_HOST) \
>     tests/test_kmem.c kernel/mm/kmem.c \
>     -o build/m8/test_kmem
> ./build/m8/test_kmem | tee build/m8/test_kmem.log

m8-audit: m8-kmem-freestanding
> $(NM) -u build/m8/kmem.freestanding.o | tee build/m8/nm_u.txt
> test ! -s build/m8/nm_u.txt
> $(READELF) -h build/m8/kmem.freestanding.o > build/m8/readelf_h.txt
> $(OBJDUMP) -dr build/m8/kmem.freestanding.o > build/m8/kmem.objdump.txt

m8-all: m8-kmem-host-test m8-audit


# ── M9 Scheduler ──────────────────────────────────────────
CC_M9    := clang
LD_M9    := ld.lld
BUILD_M9 := build/m9

CFLAGS_HOST_M9 := -std=c17 -Wall -Wextra -Werror -DMCSOS_HOST_TEST -Iinclude
CFLAGS_KERN_M9 := -target x86_64-unknown-none-elf -std=c17 -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -Wall -Wextra -Werror -Iinclude
ASFLAGS_M9     := -target x86_64-unknown-none-elf -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone

.PHONY: m9-all m9-host-test m9-freestanding m9-audit m9-clean

m9-all: m9-host-test m9-freestanding m9-audit

$(BUILD_M9):
>mkdir -p $(BUILD_M9)

m9-host-test: $(BUILD_M9)
>$(CC_M9) $(CFLAGS_HOST_M9) tests/test_scheduler.c kernel/mcsos_thread.c -o $(BUILD_M9)/m9_host_test
>$(BUILD_M9)/m9_host_test | tee $(BUILD_M9)/test_scheduler.log

m9-freestanding: $(BUILD_M9)
>$(CC_M9) $(CFLAGS_KERN_M9) -c kernel/mcsos_thread.c -o $(BUILD_M9)/mcsos_thread.freestanding.o
>$(CC_M9) $(ASFLAGS_M9) -c kernel/arch/x86_64/context_switch.S -o $(BUILD_M9)/context_switch.o
>$(LD_M9) -r $(BUILD_M9)/mcsos_thread.freestanding.o $(BUILD_M9)/context_switch.o -o $(BUILD_M9)/m9_scheduler_combined.o

m9-audit: m9-freestanding
>nm -u $(BUILD_M9)/m9_scheduler_combined.o | tee $(BUILD_M9)/nm_undefined.log
>readelf -h $(BUILD_M9)/m9_scheduler_combined.o | tee $(BUILD_M9)/readelf_header.log
>objdump -d $(BUILD_M9)/m9_scheduler_combined.o | grep -E 'mcsos_context_switch|jmp|ret|hlt' | tee $(BUILD_M9)/objdump_key.log
>sha256sum $(BUILD_M9)/m9_host_test $(BUILD_M9)/m9_scheduler_combined.o | tee $(BUILD_M9)/sha256.log

m9-clean:
>rm -rf $(BUILD_M9)

#m10 Syscal target
# M10: Syscall targets

M10_HOST_CFLAGS := \
    -std=c17 \
    -Wall \
    -Wextra \
    -Werror \
    -Iinclude \
    -Ikernel/include \
    -Ikernel/arch/x86_64/include

build/test_syscall_host: tests/test_syscall_host.c \
                         kernel/syscall/syscall.c \
                         include/mcsos/syscall.h
>mkdir -p build
>$(HOSTCC) $(M10_HOST_CFLAGS) \
>    tests/test_syscall_host.c \
>    kernel/syscall/syscall.c \
>    -o build/test_syscall_host

build/m10_syscall.o: kernel/syscall/syscall.c include/mcsos/syscall.h
>mkdir -p build
>$(CC) $(COMMON_CFLAGS) -c kernel/syscall/syscall.c -o build/m10_syscall.o

build/m10_syscall_entry.o: kernel/syscall/syscall_entry.S
>mkdir -p build
>$(CC) $(COMMON_ASFLAGS) -c kernel/syscall/syscall_entry.S \
>    -o build/m10_syscall_entry.o

build/m10_syscall_combined.o: build/m10_syscall.o build/m10_syscall_entry.o
>$(LD) -r $^ -o $@

check-m10: build/test_syscall_host build/m10_syscall_combined.o
>./build/test_syscall_host
>$(NM) -u build/m10_syscall_combined.o > build/m10_nm_undefined.txt
>test ! -s build/m10_nm_undefined.txt
>$(READELF) -h build/m10_syscall_combined.o > build/m10_readelf_header.txt
>$(OBJDUMP) -dr build/m10_syscall_combined.o > build/m10_objdump.txt
>grep -q 'Machine:.*Advanced Micro Devices X86-64' build/m10_readelf_header.txt
>grep -q 'x86_64_syscall_int80_stub' build/m10_objdump.txt
>grep -q 'iretq' build/m10_objdump.txt
>sha256sum build/test_syscall_host build/m10_syscall_combined.o \
>    > build/m10_SHA256SUMS
>echo "[PASS] M10 check selesai"

m10-host-test: build/test_syscall_host
>./build/test_syscall_host

m10-freestanding: build/m10_syscall_combined.o

m10-audit: build/m10_syscall_combined.o
>$(NM) -u build/m10_syscall_combined.o
>$(READELF) -h build/m10_syscall_combined.o
>$(OBJDUMP) -dr build/m10_syscall_combined.o | grep -E 'x86_64_syscall_int80_stub|iretq'

m10-clean:
>rm -f build/test_syscall_host build/m10_syscall.o build/m10_syscall_entry.o build/m10_syscall_combined.o build/m10_nm_undefined.txt build/m10_readelf_header.txt build/m10_objdump.txt build/m10_SHA256SUMS

