#include "mcsos/arch/io.h"
#include "mcsos/arch/pic.h"
#include "mcsos/arch/pit.h"
#include "mcsos/kernel/log.h"
#include "mcsos/kernel/panic.h"
#include "mcs_sync.h"
#include "limine.h"
#include "pmm.h"
#include "vmm.h"
#include "mcsos_thread.h"
#include "m11_elf_loader.h"
#include "mcsos/syscall.h"
#include "mcs_vfs.h"
#include "mcsos/arch/idt.h"
#include "mcsos/arch/pit.h"

/* ── Limine memory map request ─────────────────────────────────────────────
 * Limine bootloader scan ELF untuk magic bytes ini dan mengisi
 * response pointer sebelum memanggil kmain.
 * ───────────────────────────────────────────────────────────────────────── */
static volatile struct limine_memmap_request memmap_req = {
    .id = { LIMINE_COMMON_MAGIC, 0x67cf3d9d378a806f, 0xe304acdfc50c3c62 },
    .revision = 0
};

/* ── Limine HHDM request ────────────────────────────────────────────────────
 * Bootloader mengisi response->offset dengan base virtual address HHDM.
 * Digunakan VMM sebagai phys_to_virt adapter: virt = hhdm_offset + paddr.
 * ───────────────────────────────────────────────────────────────────────── */
static volatile struct limine_hhdm_request hhdm_req = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

/* ── PMM global state ───────────────────────────────────────────────────────
 * Bitmap 2MiB untuk mengelola 64GiB ruang alamat fisik (64GiB/4096B/8bit).
 * Keduanya di .bss sehingga tidak memperbesar ukuran ELF di disk.
 * ───────────────────────────────────────────────────────────────────────── */
static struct pmm_state    kernel_pmm;
static uint8_t             kernel_pmm_bitmap[PMM_BITMAP_BYTES]
    __attribute__((aligned(4096)));

/* ── VMM global state ───────────────────────────────────────────────────────
 * Satu vmm_space untuk kernel. Root page table dialokasikan dari PMM.
 * ───────────────────────────────────────────────────────────────────────── */
static struct vmm_space kernel_vmm;
static uint64_t         kernel_hhdm_offset;

static void m8_heap_bootstrap(void);
static void kernel_m11_loader_smoke(void);
static void kernel_m13_vfs_syscall_smoke(void);

/* ── External symbols dari M4/M5 ───────────────────────────────────────── */
extern void x86_64_idt_init(void);
extern void x86_64_trigger_breakpoint_test(void);
extern void serial_init(void);
extern void *memset(void *dest, int value, __SIZE_TYPE__ count);
extern void m14_block_demo_init(void);

/* ── VMM adapter: alloc frame dari PMM kernel ───────────────────────────── */
static uint64_t kernel_vmm_alloc(void *ctx) {
    (void)ctx;
    return pmm_alloc_frame(&kernel_pmm);
}

/* ── VMM adapter: free frame ke PMM kernel ──────────────────────────────── */
static void kernel_vmm_free(void *ctx, uint64_t frame_paddr) {
    (void)ctx;
    pmm_free_frame(&kernel_pmm, frame_paddr);
}

/* ── VMM adapter: phys → virt via HHDM offset ──────────────────────────── */
static void *kernel_vmm_phys_to_virt(void *ctx, uint64_t paddr) {
    (void)ctx;
    return (void *)(kernel_hhdm_offset + paddr);
}

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

/* ── VMM initialization ─────────────────────────────────────────────────────
 * Dipanggil setelah PMM siap. Ambil HHDM offset dari Limine, alokasi
 * root page table dari PMM, zero-fill, lalu inisialisasi vmm_space.
 * CR3 TIDAK diganti di sini — aktivasi page table baru adalah pengayaan
 * setelah mapping kernel/stack/IDT/serial sudah terverifikasi lengkap.
 * ───────────────────────────────────────────────────────────────────────── */
static void kernel_vmm_init(void) {
    /* Validasi HHDM response dari Limine */
    if (hhdm_req.response == (void *)0) {
        KERNEL_PANIC("limine hhdm response is NULL", 0);
    }
    kernel_hhdm_offset = hhdm_req.response->offset;
    log_key_value_hex64("[m7] vmm: hhdm_offset", kernel_hhdm_offset);

    /* Alokasi frame untuk root page table (PML4) */
    uint64_t root_paddr = pmm_alloc_frame(&kernel_pmm);
    if (root_paddr == PMM_INVALID_FRAME) {
        KERNEL_PANIC("vmm: cannot allocate root page table frame", 0);
    }
    log_key_value_hex64("[m7] vmm: root_paddr", root_paddr);

    /* Zero-fill PML4 via HHDM sebelum vmm_space_init.
     * Wajib: intermediate table baru harus bersih (VMM-I4). */
    void *root_virt = (void *)(kernel_hhdm_offset + root_paddr);
    memset(root_virt, 0, 4096);

    /* Inisialisasi vmm_space kernel */
    int rc = vmm_space_init(
        &kernel_vmm,
        root_paddr,
        (void *)0,
        kernel_vmm_alloc,
        kernel_vmm_free,
        kernel_vmm_phys_to_virt
    );
    if (rc != VMM_MAP_OK) {
        KERNEL_PANIC("vmm_space_init failed", (uint64_t)rc);
    }

    log_writeln("[m7] vmm: core initialized");
    /* Tugas wajib M7 berhenti di sini.
     * Jangan panggil vmm_write_cr3() sebelum mapping kernel/stack/
     * IDT/serial/PMM metadata sudah dipetakan dan diverifikasi. */
}

/* ── Kernel entry point ─────────────────────────────────────────────────── */

/* -- M9 Scheduler --------------------------------------------------------- */
static mcs_ramfs_t      kernel_ramfs;
static mcsos_scheduler_t g_sched;
static mcsos_thread_t    g_boot_thread;
static mcsos_thread_t    g_thread_a;
static mcsos_thread_t    g_thread_b;
static unsigned char     g_stack_a[8192] __attribute__((aligned(16)));
static unsigned char     g_stack_b[8192] __attribute__((aligned(16)));

/* Trampoline: context_switch pakai jmp bukan call.
 * Baca entry/arg dari current thread lalu panggil. */
static void m9_thread_start(void) {
    mcsos_thread_entry_t fn  = g_sched.current->entry;
    void                *arg = g_sched.current->arg;
    fn(arg);
    for (;;) { __asm__ volatile("hlt"); }
}

static void demo_thread_a(void *arg) {
    (void)arg;
    kernel_m13_vfs_syscall_smoke();
    for (;;) {
        log_writeln("[M9] thread A tick");
        mcsos_sched_yield(&g_sched);
    }
}

static void demo_thread_b(void *arg) {
    (void)arg;
    for (;;) {
        log_writeln("[M9] thread B tick");
        mcsos_sched_yield(&g_sched);
    }
}

static void kernel_m9_scheduler_init(void) {
    if (mcsos_scheduler_init(&g_sched, &g_boot_thread) != MCSOS_SCHED_OK)
        KERNEL_PANIC("M9 scheduler_init failed", 0);

    if (mcsos_thread_prepare(&g_thread_a, "demo-a", demo_thread_a, (void *)0,
                             g_stack_a, sizeof(g_stack_a),
                             g_sched.next_id++) != MCSOS_SCHED_OK)
        KERNEL_PANIC("M9 thread_prepare A failed", 0);
    g_thread_a.context.rip = (uint64_t)(uintptr_t)m9_thread_start;

    if (mcsos_thread_prepare(&g_thread_b, "demo-b", demo_thread_b, (void *)0,
                             g_stack_b, sizeof(g_stack_b),
                             g_sched.next_id++) != MCSOS_SCHED_OK)
        KERNEL_PANIC("M9 thread_prepare B failed", 0);
    g_thread_b.context.rip = (uint64_t)(uintptr_t)m9_thread_start;

    if (mcsos_sched_enqueue(&g_sched, &g_thread_a) != MCSOS_SCHED_OK)
        KERNEL_PANIC("M9 enqueue A failed", 0);
    if (mcsos_sched_enqueue(&g_sched, &g_thread_b) != MCSOS_SCHED_OK)
        KERNEL_PANIC("M9 enqueue B failed", 0);

    log_writeln("[M9] scheduler initialized");
    mcsos_sched_yield(&g_sched);
}


static mcs_fd_table_t *k_get_current_fd_table(void) {
    if (!g_sched.initialized || g_sched.current == (mcsos_thread_t *)0) return (mcs_fd_table_t *)0;
    return &g_sched.current->fd_table;
}
static mcs_ramfs_t *k_get_ramfs(void) {
    return &kernel_ramfs;
}

extern void serial_putc(char c);

static int64_t k_write_serial(const char *buf, size_t len) {
    for (size_t i = 0u; i < len; i++) serial_putc(buf[i]);
    return (int64_t)len;
}
static uint64_t k_get_ticks(void)    { return timer_ticks(); }
static void k_yield_current(void)    { mcsos_sched_yield(&g_sched); }
static void k_exit_current(int code) {
    (void)code;
    log_writeln("[M10] exit_thread stub");
    cpu_hlt();
}
static void kernel_m10_syscall_init(void) {
    mcsos_syscall_ops_t ops = {
        .get_ticks           = k_get_ticks,
        .yield_current       = k_yield_current,
        .exit_current        = k_exit_current,
        .write_serial        = k_write_serial,
        .get_current_fd_table = k_get_current_fd_table,
        .get_ramfs            = k_get_ramfs,
    };
    mcsos_syscall_init(&ops);
    mcsos_syscall_set_user_region((mcsos_user_region_t){
        .base  = 0x0000000000400000ULL,
        .limit = 0x0000800000000000ULL,
    });
    extern void x86_64_syscall_int80_stub(void);
    x86_64_idt_set_gate(0x80, (uint64_t)(uintptr_t)x86_64_syscall_int80_stub, X86_64_IDT_GATE_INTERRUPT);
    log_writeln("[M10] syscall init");
    int64_t r = mcsos_syscall_dispatch(MCSOS_SYS_PING, 0, 0, 0, 0, 0, 0);
    if (r != 0x2605020AL) KERNEL_PANIC("M10 syscall ping failed", 0);
    log_writeln("[M10] syscall ping ok");
    int64_t ticks = mcsos_syscall_dispatch(MCSOS_SYS_GET_TICKS, 0, 0, 0, 0, 0, 0);
    if (ticks < 0) KERNEL_PANIC("M10 syscall get_ticks failed", 0);
    log_writeln("[M10] syscall get_ticks ok");
    log_writeln("[M10] syscall smoke done");
}

/* ── M11: ELF loader integration smoke (konservatif) ───────────────────────
 * Bukan eksekusi process image -- cuma bangun ELF sintetis di memori,
 * panggil m11_elf64_plan_load, lalu cetak hasil plan ke serial log.
 * Alokasi frame/mapping page TIDAK dilakukan di sini (Poin 9.5: plan
 * dan eksekusi plan sengaja dipisah).
 * ───────────────────────────────────────────────────────────────────────── */
#define M11_DEMO_IMAGE_SIZE 12288u

static void m11_build_demo_image(unsigned char image[M11_DEMO_IMAGE_SIZE]) {
    memset(image, 0, M11_DEMO_IMAGE_SIZE);

    struct m11_elf64_ehdr *eh = (struct m11_elf64_ehdr *)(void *)image;
    eh->e_ident[0]  = M11_ELFMAG0;
    eh->e_ident[1]  = M11_ELFMAG1;
    eh->e_ident[2]  = M11_ELFMAG2;
    eh->e_ident[3]  = M11_ELFMAG3;
    eh->e_ident[4]  = M11_ELFCLASS64;
    eh->e_ident[5]  = M11_ELFDATA2LSB;
    eh->e_ident[6]  = M11_EV_CURRENT;
    eh->e_type      = M11_ET_EXEC;
    eh->e_machine   = M11_EM_X86_64;
    eh->e_version   = M11_EV_CURRENT;
    eh->e_entry     = 0x0000000000401000ull;
    eh->e_phoff     = sizeof(struct m11_elf64_ehdr);
    eh->e_ehsize    = sizeof(struct m11_elf64_ehdr);
    eh->e_phentsize = sizeof(struct m11_elf64_phdr);
    eh->e_phnum     = 2u;

    struct m11_elf64_phdr *ph =
        (struct m11_elf64_phdr *)(void *)(image + eh->e_phoff);

    ph[0].p_type   = M11_PT_LOAD;
    ph[0].p_flags  = M11_PF_R | M11_PF_X;
    ph[0].p_offset = 0x1000u;
    ph[0].p_vaddr  = 0x0000000000400000ull;
    ph[0].p_filesz = 16u;
    ph[0].p_memsz  = 4096u;
    ph[0].p_align  = M11_PAGE_SIZE;

    ph[1].p_type   = M11_PT_LOAD;
    ph[1].p_flags  = M11_PF_R | M11_PF_W;
    ph[1].p_offset = 0x2000u;
    ph[1].p_vaddr  = 0x0000000000401000ull;
    ph[1].p_filesz = 8u;
    ph[1].p_memsz  = 4096u;
    ph[1].p_align  = M11_PAGE_SIZE;
}

static void kernel_m11_loader_smoke(void) {
    static unsigned char demo_image[M11_DEMO_IMAGE_SIZE]
        __attribute__((aligned(16)));
    m11_build_demo_image(demo_image);

    struct m11_user_region region = {
        .base  = 0x0000000000400000ull,
        .limit = 0x0000800000000000ull,
    };

    struct m11_process_image_plan plan;
    int rc = m11_elf64_plan_load(demo_image, M11_DEMO_IMAGE_SIZE, region, &plan);

    if (rc != M11_OK) {
        log_writeln("[M11] elf: plan FAILED");
        log_writeln(m11_error_name(rc));
        return;
    }

    log_writeln("[M11] elf: ident ok");
    log_key_value_hex64("[M11] elf: phnum", plan.segment_count);

    for (uint32_t i = 0u; i < plan.segment_count; i++) {
        log_key_value_hex64("[M11] elf: segment vaddr",  plan.segments[i].vaddr);
        log_key_value_hex64("[M11] elf: segment filesz", plan.segments[i].filesz);
        log_key_value_hex64("[M11] elf: segment memsz",  plan.segments[i].memsz);
        log_key_value_hex64("[M11] elf: segment flags",  plan.segments[i].flags);
    }

    log_key_value_hex64("[M11] elf: plan ok entry", plan.entry);
    log_writeln("[M11] user image plan ready");
}

static mcs_spinlock_t boot_stats_lock;
static mcs_lockdep_state_t boot_lockdep;
static uint64_t boot_counter;

void m12_sync_selftest(void) {
    mcs_lockdep_init(&boot_lockdep);
    mcs_spin_init(&boot_stats_lock, 10u, "boot_stats");

    if (mcs_lockdep_before_acquire(&boot_lockdep, 10u, "boot_stats") != MCS_SYNC_OK) {
        KERNEL_PANIC("M12 lockdep acquire failed", 0xC12A0001u);
    }

    mcs_spin_lock(&boot_stats_lock);
    boot_counter++;
    mcs_spin_unlock(&boot_stats_lock);

    if (mcs_lockdep_after_release(&boot_lockdep, 10u, "boot_stats") != MCS_SYNC_OK) {
        KERNEL_PANIC("M12 lockdep release failed", 0xC12A0002u);
    }

    log_writeln("[M12] sync selftest passed");
}


/* ── M13: VFS syscall smoke test (real dispatch, bukan panggil mcs_vfs_* langsung) ──
 * Catatan simplifikasi: user_region sementara di-override untuk mencakup
 * alamat buffer kernel statis di bawah, karena belum ada user page table
 * nyata di titik boot ini. Region asli M11 dipulihkan setelah test.
 * ───────────────────────────────────────────────────────────────────────── */
static void kernel_m13_vfs_syscall_smoke(void) {
    static char          smoke_path[16] = "/hello.txt";
    static unsigned char smoke_buf[16];

    mcsos_syscall_set_user_region((mcsos_user_region_t){
        .base  = 0x1ULL,
        .limit = 0xFFFFFFFFFFFFFFFEULL,
    });

    int64_t fd = mcsos_syscall_dispatch(MCSOS_SYS_OPEN,
                                         (uint64_t)(uintptr_t)smoke_path,
                                         10u,
                                         (uint64_t)MCS_O_RDONLY,
                                         0, 0, 0);
    if (fd < 0) KERNEL_PANIC("M13 syscall open failed", (uint64_t)fd);
    log_key_value_hex64("[M13] syscall: open fd", (uint64_t)fd);

    int64_t n = mcsos_syscall_dispatch(MCSOS_SYS_READ, (uint64_t)fd,
                                        (uint64_t)(uintptr_t)smoke_buf, 5u,
                                        0, 0, 0);
    if (n != 5) KERNEL_PANIC("M13 syscall read failed", (uint64_t)n);
    if (smoke_buf[0] != 'h' || smoke_buf[1] != 'e' || smoke_buf[2] != 'l'
     || smoke_buf[3] != 'l' || smoke_buf[4] != 'o') {
        KERNEL_PANIC("M13 syscall read data mismatch", 0);
    }
    log_writeln("[M13] syscall: read data verified");

    int64_t seek_r = mcsos_syscall_dispatch(MCSOS_SYS_LSEEK, (uint64_t)fd,
                                             0, MCS_SEEK_SET, 0, 0, 0);
    if (seek_r != 0) KERNEL_PANIC("M13 syscall lseek failed", (uint64_t)seek_r);

    int64_t close_r = mcsos_syscall_dispatch(MCSOS_SYS_CLOSE, (uint64_t)fd,
                                              0, 0, 0, 0, 0);
    if (close_r != MCS_OK) KERNEL_PANIC("M13 syscall close failed", (uint64_t)close_r);
    log_writeln("[M13] syscall: close ok");
    log_writeln("[M13] syscall smoke done");

    mcsos_syscall_set_user_region((mcsos_user_region_t){
        .base  = 0x0000000000400000ULL,
        .limit = 0x0000800000000000ULL,
    });
}

void kmain(void) {
    cpu_cli();
    serial_init();

    log_writeln("[MCSOS:M7] boot: memory manager bring-up start");

    x86_64_idt_init();
    log_writeln("[MCSOS:M7] idt: loaded");

    pic_remap(PIC_MASTER_OFFSET, PIC_SLAVE_OFFSET);
    pic_mask_all();
    pic_unmask_irq(0);
    log_writeln("[MCSOS:M7] pic: remapped and masked");

    pit_configure_hz(100u);
    log_writeln("[MCSOS:M7] pit: configured 100Hz");

    /* PMM harus diinisialisasi sebelum sti() agar IRQ tidak memicu
     * alokasi sebelum bitmap siap. */
    kernel_memory_init();
    log_writeln("[MCSOS:M7] pmm: ready");

    /* VMM diinisialisasi setelah PMM siap, sebelum sti(). */
    kernel_vmm_init();
    log_writeln("[MCSOS:M7] vmm: ready");

    m8_heap_bootstrap();
    log_writeln("[MCSOS:M8] heap: ready");

    m14_block_demo_init();
    log_writeln("[M14] block: ram0 registered");
    log_writeln("[MCSOS:M7] sti: enabling interrupts");
    cpu_sti();

    mcs_ramfs_init(&kernel_ramfs);
    mcs_ramfs_seed_file(&kernel_ramfs, "/hello.txt", (const uint8_t *)"hello-mcsos", 11);
    log_writeln("[M13] ramfs: initialized + demo file seeded");

    kernel_m10_syscall_init();
    kernel_m11_loader_smoke();
    m12_sync_selftest();
    kernel_m9_scheduler_init();
    log_writeln("[M9] boot idle: hlt loop");

#if defined(MCSOS_M4_TRIGGER_BREAKPOINT)
    x86_64_trigger_breakpoint_test();
#endif

    for (;;) {
        cpu_hlt();
    }
}

/* ── M8: Early kernel heap bootstrap ───────────────────────────────────────
 * Arena statik di .bss, sudah terpetakan oleh Limine sebelum kmain.
 * Dipanggil setelah PMM dan VMM siap, sebelum sti().
 * ───────────────────────────────────────────────────────────────────────── */
#include "mcsos/kmem.h"

#define M8_BOOT_HEAP_SIZE (64u * 1024u)
static unsigned char m8_boot_heap[M8_BOOT_HEAP_SIZE] __attribute__((aligned(4096)));

static void m8_heap_bootstrap(void) {
    int rc = kmem_init(m8_boot_heap, sizeof(m8_boot_heap));
    if (rc != 0) {
        KERNEL_PANIC("M8 kmem_init failed", (uint64_t)rc);
    }

    void *probe = kmem_alloc(128);
    if (probe == (void *)0) {
        KERNEL_PANIC("M8 kmem_alloc probe failed", 0);
    }

    if (kmem_free_checked(probe) != 0) {
        KERNEL_PANIC("M8 kmem_free_checked probe failed", 0);
    }

    kmem_stats_t st;
    kmem_get_stats(&st);
    log_writeln("[m8] kmem: initialized");
    log_key_value_hex64("[m8] kmem: total_bytes", (uint64_t)st.total_bytes);
    log_key_value_hex64("[m8] kmem: free_bytes",  (uint64_t)st.free_bytes);
    log_key_value_hex64("[m8] kmem: largest_free",(uint64_t)st.largest_free);
    log_key_value_hex64("[m8] kmem: block_count", (uint64_t)st.block_count);
}
