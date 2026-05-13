#include <stdint.h>
#include <stddef.h>
#include <mcsos/arch/io.h>

#define COM1_PORT 0x3F8u

static int serial_transmit_empty(void) {
    return (inb((uint16_t)(COM1_PORT + 5u)) & 0x20u) != 0;
}

void serial_init(void) {
    outb((uint16_t)(COM1_PORT + 1u), 0x00u);
    outb((uint16_t)(COM1_PORT + 3u), 0x80u);
    outb((uint16_t)(COM1_PORT + 0u), 0x03u);
    outb((uint16_t)(COM1_PORT + 1u), 0x00u);
    outb((uint16_t)(COM1_PORT + 3u), 0x03u);
    outb((uint16_t)(COM1_PORT + 2u), 0xC7u);
    outb((uint16_t)(COM1_PORT + 4u), 0x0Bu);
}

void serial_putc(char c) {
    if (c == '\n') {
        serial_putc('\r');
    }
    while (!serial_transmit_empty()) { }
    outb((uint16_t)COM1_PORT, (uint8_t)c);
}

void serial_write(const char *s) {
    if (s == (const char *)0) {
        return;
    }
    while (*s != '\0') {
        serial_putc(*s++);
    }
}
