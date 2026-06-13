#include <stdint.h>
#include <mcsos/arch/idt.h>
#include <mcsos/kernel/log.h>
#include <mcsos/kernel/panic.h>
#include <mcsos/arch/pic.h>
#include "vmm.h"

static const char *exception_names[32] = {
    "#DE Divide Error",
    "#DB Debug",
    "NMI Interrupt",
    "#BP Breakpoint",
    "#OF Overflow",
    "#BR Bound Range Exceeded",
    "#UD Invalid Opcode",
    "#NM Device Not Available",
    "#DF Double Fault",
    "Coprocessor Segment Overrun",
    "#TS Invalid TSS",
    "#NP Segment Not Present",
    "#SS Stack Segment Fault",
    "#GP General Protection Fault",
    "#PF Page Fault",
    "Reserved",
    "#MF x87 Floating-Point Exception",
    "#AC Alignment Check",
    "#MC Machine Check",
    "#XM SIMD Floating-Point Exception",
    "#VE Virtualization Exception",
    "#CP Control Protection Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "#HV Hypervisor Injection Exception",
    "#VC VMM Communication Exception",
    "#SX Security Exception",
    "Reserved"
};

static uint64_t trap_count;

static const char *trap_name(uint64_t vector) {
    if (vector < 32u) {
        return exception_names[vector];
    }
    return "external-or-user-defined-interrupt";
}

uint64_t m4_trap_count_for_test(void) {
    return trap_count;
}

static void log_trap_frame(const x86_64_trap_frame_t *frame) {
    log_key_value_hex64("trap_vector", frame->vector);
    log_key_value_hex64("trap_error", frame->error_code);
    log_key_value_hex64("trap_rip", frame->rip);
    log_key_value_hex64("trap_cs", frame->cs);
    log_key_value_hex64("trap_rflags", frame->rflags);
    log_key_value_hex64("trap_rax", frame->rax);
    log_key_value_hex64("trap_rbx", frame->rbx);
    log_key_value_hex64("trap_rcx", frame->rcx);
    log_key_value_hex64("trap_rdx", frame->rdx);
}

/* Decode dan dump diagnostics page fault (#PF, vector 14).
 *
 * RSP: struct trap_frame tidak menyimpan RSP (CPU hanya push RSP ke stack
 * saat privilege-level change, i.e. user->kernel). Untuk kernel-mode #PF,
 * RSP saat exception bisa diestimasikan dari alamat slot tepat setelah
 * rflags di stack frame yang di-push isr_common_stub. Frame di-pass via
 * %rdi sebagai pointer, jadi (frame + 1) menunjuk ke lokasi tersebut.
 * Ini cukup untuk keperluan diagnostics M7 tanpa modifikasi isr.S.
 */
static void page_fault_dump(const x86_64_trap_frame_t *frame) {
    uint64_t cr2        = vmm_read_cr2();
    uint64_t error_code = frame->error_code;
    /* estimasi RSP kernel-mode: slot tepat di atas rflags di stack */
    uint64_t rsp_est    = (uint64_t)(frame + 1);

    log_writeln("[M7] #PF page fault diagnostics:");
    log_key_value_hex64("pf_cr2",        cr2);
    log_key_value_hex64("pf_error_code", error_code);
    log_key_value_hex64("pf_rip",        frame->rip);
    log_key_value_hex64("pf_rsp_est",    rsp_est);

    /* decode bit error_code:
     *   bit 0 (P)    : 0=non-present page, 1=protection violation
     *   bit 1 (W/R)  : 0=read,             1=write
     *   bit 2 (U/S)  : 0=supervisor,       1=user mode
     *   bit 3 (RSVD) : 1=reserved bit set in PTE
     *   bit 4 (I/D)  : 0=data access,      1=instruction fetch
     */
    log_write("[M7] pf_present=");
    log_write((error_code & (1u << 0)) ? "1(protection)" : "0(non-present)");
    log_putc('\n');

    log_write("[M7] pf_write=");
    log_write((error_code & (1u << 1)) ? "1(write)" : "0(read)");
    log_putc('\n');

    log_write("[M7] pf_user=");
    log_write((error_code & (1u << 2)) ? "1(user)" : "0(supervisor)");
    log_putc('\n');

    log_write("[M7] pf_rsvd=");
    log_write((error_code & (1u << 3)) ? "1(reserved-bit-set)" : "0");
    log_putc('\n');

    log_write("[M7] pf_id=");
    log_write((error_code & (1u << 4)) ? "1(instruction-fetch)" : "0(data-access)");
    log_putc('\n');
}

void x86_64_trap_dispatch(x86_64_trap_frame_t *frame) {
    KERNEL_ASSERT(frame != (x86_64_trap_frame_t *)0);
    ++trap_count;

    log_write("[M4] trap dispatch: ");
    log_writeln(trap_name(frame->vector));
    log_trap_frame(frame);

    if (frame->vector == 3u) {
        log_writeln("[M4] breakpoint handled; returning with iretq");
        return;
    }

    if (frame->vector == 14u) {
        page_fault_dump(frame);
        KERNEL_PANIC("unrecoverable page fault", frame->error_code);
    }

    if (frame->vector >= 32u && frame->vector <= 47u) {
        pic_send_eoi(frame->vector - 32u);
        return;
    }

    KERNEL_PANIC("unrecoverable CPU exception", frame->vector);
}
