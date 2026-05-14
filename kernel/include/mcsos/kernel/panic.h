#ifndef MCSOS_KERNEL_PANIC_H
#define MCSOS_KERNEL_PANIC_H

#include <stdint.h>

__attribute__((noreturn)) void kernel_panic_at(const char *file, int line, const char *reason, uint64_t code);

#define KERNEL_PANIC(reason, code) kernel_panic_at(__FILE__, __LINE__, (reason), (uint64_t)(code))
#define KERNEL_ASSERT(expr) do { \
    if (!(expr)) { \
        kernel_panic_at(__FILE__, __LINE__, "assertion failed: " #expr, 0xA55E4710u); \
    } \
} while (0)

#endif
