#ifndef MCSOS_PIC_H
#define MCSOS_PIC_H
#include <stdint.h>

#define PIC_MASTER_OFFSET 0x20u
#define PIC_SLAVE_OFFSET  0x28u

void pic_remap(uint8_t master_offset, uint8_t slave_offset);
void pic_mask_all(void);
void pic_unmask_irq(uint8_t irq);
void pic_send_eoi(uint8_t irq);
uint8_t pic_read_master_mask(void);
uint8_t pic_read_slave_mask(void);

#endif
