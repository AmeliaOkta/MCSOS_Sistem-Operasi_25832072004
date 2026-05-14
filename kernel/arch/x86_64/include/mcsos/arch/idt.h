#ifndef MCSOS_ARCH_IDT_H
#define MCSOS_ARCH_IDT_H

#include <stdint.h>

#define X86_64_IDT_VECTOR_COUNT 256u
#define X86_64_KERNEL_CODE_SELECTOR 0x28u
#define X86_64_IDT_GATE_INTERRUPT 0x8Eu
#define X86_64_IDT_GATE_TRAP 0x8Fu

typedef struct __attribute__((packed)) {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attributes;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} x86_64_idt_entry_t;

typedef struct __attribute__((packed)) {
    uint16_t limit;
    uint64_t base;
} x86_64_idtr_t;

typedef struct __attribute__((packed)) {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rbp;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
    uint64_t vector;
    uint64_t error_code;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
} x86_64_trap_frame_t;

void x86_64_idt_init(void);
void x86_64_idt_set_gate(uint8_t vector, uint64_t handler, uint8_t type_attributes);
void x86_64_trap_dispatch(x86_64_trap_frame_t *frame);
uint64_t x86_64_idt_base_for_test(void);
uint16_t x86_64_idt_limit_for_test(void);
void x86_64_trigger_breakpoint_for_test(void);

#endif
