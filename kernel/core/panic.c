#include <stdint.h>
#include <mcsos/arch/cpu.h>
#include <mcsos/kernel/log.h>
#include <mcsos/kernel/panic.h>
#include <mcsos/kernel/version.h>

static void log_dec_u32(uint32_t value) {
    char buf[11];
    uint32_t i = 0u;

    if (value == 0u) {
        log_putc('0');
        return;
    }

    while (value != 0u && i < sizeof(buf)) {
        buf[i++] = (char)('0' + (value % 10u));
        value /= 10u;
    }

    while (i != 0u) {
        log_putc(buf[--i]);
    }
}

__attribute__((noreturn)) void kernel_panic_at(const char *file, int line, const char *reason, uint64_t code) {
    uint64_t rflags = cpu_read_rflags();

    cpu_cli();
    log_writeln("");
    log_writeln("================ MCSOS KERNEL PANIC ================");
    log_write("system=");
    log_write(MCSOS_NAME);
    log_write(" version=");
    log_write(MCSOS_VERSION);
    log_write(" milestone=");
    log_writeln(MCSOS_MILESTONE);
    log_write("reason=");
    log_writeln(reason != (const char *)0 ? reason : "<null>");
    log_write("location=");
    log_write(file != (const char *)0 ? file : "<unknown>");
    log_write(":");
    log_dec_u32((uint32_t)line);
    log_putc('\n');
    log_key_value_hex64("panic_code", code);
    log_key_value_hex64("rflags_before_cli", rflags);
    log_writeln("state=halted");
    log_writeln("====================================================");

    cpu_halt_forever();
}
