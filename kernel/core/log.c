#include <stdint.h>
#include <mcsos/kernel/log.h>

void serial_init(void);
void serial_putc(char c);
void serial_write(const char *s);

static int g_log_ready = 0;

void log_init(void) {
    serial_init();
    g_log_ready = 1;
}

void log_putc(char c) {
    if (g_log_ready == 0) {
        serial_init();
        g_log_ready = 1;
    }
    serial_putc(c);
}

void log_write(const char *s) {
    if (g_log_ready == 0) {
        serial_init();
        g_log_ready = 1;
    }
    serial_write(s);
}

void log_writeln(const char *s) {
    log_write(s);
    log_putc('\n');
}

void log_hex64(uint64_t value) {
    static const char digits[] = "0123456789abcdef";
    log_write("0x");
    for (int shift = 60; shift >= 0; shift -= 4) {
        uint8_t nibble = (uint8_t)((value >> (uint32_t)shift) & 0x0Fu);
        log_putc(digits[nibble]);
    }
}

void log_key_value_hex64(const char *key, uint64_t value) {
    log_write(key);
    log_write("=");
    log_hex64(value);
    log_putc('\n');
}
