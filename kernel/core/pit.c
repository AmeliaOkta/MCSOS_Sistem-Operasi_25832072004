#include <mcsos/arch/io.h>
#include <mcsos/arch/pit.h>

#define PIT_CHANNEL0 0x40u
#define PIT_COMMAND  0x43u

static volatile uint64_t g_ticks = 0;

void pit_configure_hz(uint32_t hz) {
    if (hz == 0u) {
        hz = 100u;
    }
    uint32_t divisor = PIT_BASE_FREQUENCY_HZ / hz;
    if (divisor == 0u) {
        divisor = 1u;
    }
    if (divisor > 0xFFFFu) {
        divisor = 0xFFFFu;
    }

    outb(PIT_COMMAND, 0x36u);
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFFu));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8u) & 0xFFu));
}

uint64_t timer_ticks(void) {
    return g_ticks;
}

void timer_on_irq0(void) {
    ++g_ticks;
    if ((g_ticks % 100u) == 0u) {
    }
}
