#include "mcsos/arch/io.h"

#define X86_64_IDT_VECTOR_COUNT 256
#define X86_64_IDT_GATE_INTERRUPT 0x8E
#define X86_64_IDT_GATE_TRAP      0xEF

struct x86_64_idt_entry_t {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} __attribute__((packed));

struct x86_64_idtr_t {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

extern void *isr_stub_table[];

static struct x86_64_idt_entry_t idt[X86_64_IDT_VECTOR_COUNT];
static struct x86_64_idtr_t idtr;

void x86_64_idt_set_gate(uint8_t vector, uint64_t handler, uint8_t type_attr) {
    idt[vector].offset_low = (uint16_t)(handler & 0xFFFF);
    idt[vector].selector = x86_64_read_cs();
    idt[vector].ist = 0;
    idt[vector].type_attr = type_attr;
    idt[vector].offset_mid = (uint16_t)((handler >> 16) & 0xFFFF);
    idt[vector].offset_high = (uint32_t)((handler >> 32) & 0xFFFFFFFF);
    idt[vector].zero = 0;
}

static void lidt(const struct x86_64_idtr_t *ptr) {
    __asm__ volatile ("lidt (%0)" :: "r"(ptr) : "memory");
}

void x86_64_idt_init(void) {
    for (uint16_t i = 0; i < X86_64_IDT_VECTOR_COUNT; ++i) {
        x86_64_idt_set_gate((uint8_t)i, 0, 0);
    }

    for (uint8_t vector = 0; vector < 48; ++vector) {
        uint8_t gate_type = X86_64_IDT_GATE_INTERRUPT;
        if (vector == 3) {
            gate_type = X86_64_IDT_GATE_TRAP;
        }
        x86_64_idt_set_gate(vector, (uint64_t) isr_stub_table[vector], gate_type);
    }

    idtr.limit = (uint16_t)(sizeof(idt) - 1);
    idtr.base = (uint64_t)(uintptr_t)&idt[0];

    lidt(&idtr);
}
