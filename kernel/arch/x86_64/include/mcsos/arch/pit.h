#ifndef MCSOS_PIT_H
#define MCSOS_PIT_H
#include <stdint.h>

#define PIT_BASE_FREQUENCY_HZ 1193182u

void pit_configure_hz(uint32_t hz);
uint64_t timer_ticks(void);
void timer_on_irq0(void);

#endif
