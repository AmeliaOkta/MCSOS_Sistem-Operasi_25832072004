#include "mcsos/arch/io.h"
#include "mcsos/arch/pic.h"
#include "mcsos/arch/pit.h"
#include "mcsos/arch/pic.h"
#include <stdint.h>

extern void x86_64_idt_init(void);
extern void x86_64_trigger_breakpoint_test(void);
extern void serial_init(void);
extern void log_writeln(const char *str);

void kmain(void) {
    cpu_cli();
    
    serial_init();
    log_writeln("[MCSOS:M5] boot: external interrupt bring-up start");
    
    x86_64_idt_init();
    log_writeln("[MCSOS:M5] idt: loaded");
    
    pic_remap(PIC_MASTER_OFFSET, PIC_SLAVE_OFFSET);
    pic_mask_all();
    pic_unmask_irq(0);
    log_writeln("[MCSOS:M5] pic: remapped and masked");
    
    pit_configure_hz(100u);
    log_writeln("[MCSOS:M5] pit: configured 100Hz");
    log_writeln("[MCSOS:M5] sti: enabling interrupts");
    
    cpu_sti();

#if defined(MCSOS_TEST_BREAKPOINT)
    x86_64_trigger_breakpoint_test();
#endif

    for (;;) {
        cpu_hlt();
    }
}
