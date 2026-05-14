#include <stdint.h>
#include <mcsos/arch/idt.h>
#include <mcsos/arch/isr.h>
#include <mcsos/kernel/log.h>
#include <mcsos/kernel/panic.h>

static x86_64_idt_entry_t idt[X86_64_IDT_VECTOR_COUNT];
static x86_64_idtr_t idtr;

static inline void lidt(const x86_64_idtr_t *descriptor) {
    __asm__ volatile ("lidt (%0)" :: "r"(descriptor) : "memory");
}

void x86_64_idt_set_gate(uint8_t vector, uint64_t handler, uint8_t type_attributes) {
    idt[vector].offset_low = (uint16_t)(handler & 0xFFFFu);
    idt[vector].selector = (uint16_t)X86_64_KERNEL_CODE_SELECTOR;
    idt[vector].ist = 0u;
    idt[vector].type_attributes = type_attributes;
    idt[vector].offset_mid = (uint16_t)((handler >> 16u) & 0xFFFFu);
    idt[vector].offset_high = (uint32_t)((handler >> 32u) & 0xFFFFFFFFu);
    idt[vector].reserved = 0u;
}

uint64_t x86_64_idt_base_for_test(void) {
    return idtr.base;
}

uint16_t x86_64_idt_limit_for_test(void) {
    return idtr.limit;
}

void x86_64_idt_init(void) {
    for (uint16_t i = 0u; i < X86_64_IDT_VECTOR_COUNT; ++i) {
        x86_64_idt_set_gate((uint8_t)i, 0u, 0u);
    }

    for (uint8_t vector = 0u; vector < 32u; ++vector) {
        uint8_t gate_type = X86_64_IDT_GATE_INTERRUPT;
        if (vector == 3u) {
            gate_type = X86_64_IDT_GATE_TRAP;
        }
        x86_64_idt_set_gate(vector, (uint64_t)(uintptr_t)x86_64_exception_stubs[vector], gate_type);
    }

    idtr.limit = (uint16_t)(sizeof(idt) - 1u);
    idtr.base = (uint64_t)(uintptr_t)&idt[0];

    KERNEL_ASSERT(sizeof(x86_64_idt_entry_t) == 16u);
    KERNEL_ASSERT(idtr.limit == (uint16_t)((X86_64_IDT_VECTOR_COUNT * sizeof(x86_64_idt_entry_t)) - 1u));
    lidt(&idtr);
    log_key_value_hex64("idt_base", idtr.base);
    log_key_value_hex64("idt_limit", (uint64_t)idtr.limit);
    log_writeln("[M4] IDT loaded");
}

void x86_64_trigger_breakpoint_for_test(void) {
    __asm__ volatile ("int3");
}
