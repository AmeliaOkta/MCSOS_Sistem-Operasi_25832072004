#include "mcsos/arch/io.h"
#include "mcsos/arch/pic.h"

#define PIC1_COMMAND 0x20u
#define PIC1_DATA    0x21u
#define PIC2_COMMAND 0xA0u
#define PIC2_DATA    0xA1u
#define PIC_EOI      0x20u

void pic_remap(uint8_t master_offset, uint8_t slave_offset) {
    uint8_t master_mask = inb(PIC1_DATA);
    uint8_t slave_mask = inb(PIC2_DATA);

    outb(PIC1_COMMAND, 0x11u);
    io_wait();
    outb(PIC2_COMMAND, 0x11u);
    io_wait();

    outb(PIC1_DATA, master_offset);
    io_wait();
    outb(PIC2_DATA, slave_offset);
    io_wait();

    outb(PIC1_DATA, 0x04u);
    io_wait();
    outb(PIC2_DATA, 0x02u);
    io_wait();

    outb(PIC1_DATA, 0x01u);
    io_wait();
    outb(PIC2_DATA, 0x01u);
    io_wait();

    outb(PIC1_DATA, master_mask);
    outb(PIC2_DATA, slave_mask);
}

void pic_mask_all(void) {
    outb(PIC1_DATA, 0xFFu);
    outb(PIC2_DATA, 0xFFu);
}

void pic_unmask_irq(uint8_t irq) {
    uint16_t port;
    uint8_t line;
    if (irq < 8u) {
        port = PIC1_DATA;
        line = irq;
    } else {
        port = PIC2_DATA;
        line = (uint8_t)(irq - 8u);
    }
    uint8_t mask = inb(port);
    mask = (uint8_t)(mask & (uint8_t)~(1u << line));
    outb(port, mask);
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8u) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

uint8_t pic_read_master_mask(void) {
    return inb(PIC1_DATA);
}

uint8_t pic_read_slave_mask(void) {
    return inb(PIC2_DATA);
}
