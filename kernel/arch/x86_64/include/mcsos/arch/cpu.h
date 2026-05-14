#ifndef MCSOS_ARCH_CPU_H
#define MCSOS_ARCH_CPU_H

#include <stdint.h>

static inline void cpu_cli(void) {
    __asm__ volatile ("cli" : : : "memory");
}

static inline void cpu_hlt(void) {
    __asm__ volatile ("hlt" : : : "memory");
}

static inline void cpu_pause(void) {
    __asm__ volatile ("pause" : : : "memory");
}

static inline void cpu_breakpoint(void) {
    __asm__ volatile ("int3" : : : "memory");
}

static inline uint64_t cpu_read_rflags(void) {
    uint64_t flags;
    __asm__ volatile ("pushfq; popq %0" : "=r"(flags) : : "memory");
    return flags;
}

__attribute__((noreturn)) static inline void cpu_halt_forever(void) {
    cpu_cli();
    for (;;) {
        cpu_hlt();
    }
}

#endif
