#ifndef MCSOS_IO_H
#define MCSOS_IO_H
#include <stdint.h>

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" :: "a"(value), "Nd"(port) : "memory");
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port) : "memory");
    return value;
}

static inline void io_wait(void) {
    outb(0x80, 0);
}

static inline void cpu_cli(void) {
    __asm__ volatile ("cli" ::: "memory");
}

static inline void cpu_sti(void) {
    __asm__ volatile ("sti" ::: "memory");
}

static inline void cpu_hlt(void) {
    __asm__ volatile ("hlt" ::: "memory");
}

static inline uint16_t x86_64_read_cs(void) {
    uint16_t value;
    __asm__ volatile ("movw %%cs, %0" : "=rm"(value));
    return value;
}

#endif
