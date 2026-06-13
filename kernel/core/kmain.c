#include "mcsos/arch/io.h"
#include "mcsos/arch/pic.h"
#include "mcsos/arch/pit.h"
#include "mcsos/kernel/log.h"
#include "mcsos/kernel/panic.h"
#include "limine.h"
#include "pmm.h"

/* ── Limine memory map request ─────────────────────────────────────────────
 * Limine bootloader scan ELF untuk magic bytes ini dan mengisi
 * response pointer sebelum memanggil kmain.
 * ───────────────────────────────────────────────────────────────────────── */
static volatile struct limine_memmap_request memmap_req = {
    .id = { LIMINE_COMMON_MAGIC, 0x67cf3d9d378a806f, 0xe304acdfc50c3c62 },
    .revision = 0
};

/* ── PMM global state ───────────────────────────────────────────────────────
 * Bitmap 2MiB untuk mengelola 64GiB ruang alamat fisik (64GiB/4096B/8bit).
 * Keduanya di .bss sehingga tidak memperbesar ukuran ELF di disk.
 * ───────────────────────────────────────────────────────────────────────── */
static struct pmm_state    kernel_pmm;
static uint8_t             kernel_pmm_bitmap[PMM_BITMAP_BYTES]
    __attribute__((aligned(4096)));

/* ── External symbols dari M4/M5 ───────────────────────────────────────── */
extern void x86_64_idt_init(void);
extern void x86_64_trigger_breakpoint_test(void);
extern void serial_init(void);

/* ── Adapter: Limine memmap type → MCSOS boot_mem_type ─────────────────────
 * Default-nya BOOT_MEM_RESERVED (fail-closed): tipe tidak dikenal
 * tidak akan pernah dibuka sebagai frame bebas.
 * ───────────────────────────────────────────────────────────────────────── */
static uint32_t limine_type_to_boot_mem(uint64_t limine_type) {
    switch (limine_type) {
        case LIMINE_MEMMAP_USABLE:
            return BOOT_MEM_USABLE;
        case LIMINE_MEMMAP_RESERVED:
            return BOOT_MEM_RESERVED;
        case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
            return BOOT_MEM_ACPI_RECLAIMABLE;
        case LIMINE_MEMMAP_ACPI_NVS:
            return BOOT_MEM_ACPI_NVS;
        case LIMINE_MEMMAP_BAD_MEMORY:
            return BOOT_MEM_BAD_MEMORY;
        case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
            return BOOT_MEM_BOOTLOADER_RECLAIMABLE;
        case LIMINE_MEMMAP_KERNEL_AND_MODULES:
            return BOOT_MEM_KERNEL_AND_MODULES;
        case LIMINE_MEMMAP_FRAMEBUFFER:
            return BOOT_MEM_FRAMEBUFFER;
        default:
            return BOOT_MEM_RESERVED;
    }
}

/* ── PMM initialization ─────────────────────────────────────────────────────
 * Dipanggil setelah serial/panic/IDT siap, sebelum sti().
 * Urutan penting: konversi entry → init PMM → log statistik → smoke test.
 * ───────────────────────────────────────────────────────────────────────── */
#define MAX_BOOT_REGIONS 64U

static void kernel_memory_init(void) {
    if (memmap_req.response == (void *)0) {
        KERNEL_PANIC("limine memmap response is NULL", 0);
    }

    struct limine_memmap_response *resp = memmap_req.response;

    static struct boot_mem_region regions[MAX_BOOT_REGIONS];
    uint64_t count = resp->entry_count;
    if (count > MAX_BOOT_REGIONS) {
        count = MAX_BOOT_REGIONS;
    }

    for (uint64_t i = 0; i < count; i++) {
        struct limine_memmap_entry *e = resp->entries[i];
        regions[i].base   = e->base;
        regions[i].length = e->length;
        regions[i].type   = limine_type_to_boot_mem(e->type);
    }

    bool ok = pmm_init_from_map(
        &kernel_pmm,
        regions,
        (size_t)count,
        kernel_pmm_bitmap,
        sizeof(kernel_pmm_bitmap),
        PMM_MAX_PHYS_BYTES
    );
    if (!ok) {
        KERNEL_PANIC("pmm_init_from_map failed", 0);
    }

    log_writeln("[m6] pmm: initialized");
    log_key_value_hex64("[m6] pmm: frame_count", pmm_frame_count(&kernel_pmm));
    log_key_value_hex64("[m6] pmm: free_frames", pmm_free_count(&kernel_pmm));
    log_key_value_hex64("[m6] pmm: used_frames", pmm_used_count(&kernel_pmm));

    /* Smoke test: alokasi satu frame lalu bebaskan kembali */
    uint64_t f = pmm_alloc_frame(&kernel_pmm);
    if (f == PMM_INVALID_FRAME) {
        KERNEL_PANIC("pmm_alloc_frame returned invalid frame", 0);
    }
    log_key_value_hex64("[m6] pmm: sample_frame", f);

    if (!pmm_free_frame(&kernel_pmm, f)) {
        KERNEL_PANIC("pmm_free_frame failed", 0);
    }
    log_writeln("[m6] pmm: smoke test passed");
}

/* ── Kernel entry point ─────────────────────────────────────────────────── */
void kmain(void) {
    cpu_cli();
    serial_init();

    log_writeln("[MCSOS:M6] boot: memory manager bring-up start");

    x86_64_idt_init();
    log_writeln("[MCSOS:M6] idt: loaded");

    pic_remap(PIC_MASTER_OFFSET, PIC_SLAVE_OFFSET);
    pic_mask_all();
    pic_unmask_irq(0);
    log_writeln("[MCSOS:M6] pic: remapped and masked");

    pit_configure_hz(100u);
    log_writeln("[MCSOS:M6] pit: configured 100Hz");

    /* PMM harus diinisialisasi sebelum sti() agar IRQ tidak memicu
     * alokasi sebelum bitmap siap. */
    kernel_memory_init();
    log_writeln("[MCSOS:M6] pmm: ready");

    log_writeln("[MCSOS:M6] sti: enabling interrupts");
    cpu_sti();

#if defined(MCSOS_M4_TRIGGER_BREAKPOINT)
    x86_64_trigger_breakpoint_test();
#endif

    for (;;) {
        cpu_hlt();
    }
}
