#ifndef MCSOS_PMM_H
#define MCSOS_PMM_H

#include "types.h"

#define PMM_PAGE_SIZE 4096ULL
#define PMM_MAX_PHYS_BYTES (64ULL * 1024ULL * 1024ULL * 1024ULL)
#define PMM_MAX_FRAMES (PMM_MAX_PHYS_BYTES / PMM_PAGE_SIZE)
#define PMM_BITMAP_BYTES (PMM_MAX_FRAMES / 8ULL)
#define PMM_INVALID_FRAME 0xffffffffffffffffULL

enum boot_mem_type {
    BOOT_MEM_USABLE = 1,
    BOOT_MEM_RESERVED = 2,
    BOOT_MEM_BOOTLOADER_RECLAIMABLE = 3,
    BOOT_MEM_KERNEL_AND_MODULES = 4,
    BOOT_MEM_FRAMEBUFFER = 5,
    BOOT_MEM_ACPI_RECLAIMABLE = 6,
    BOOT_MEM_ACPI_NVS = 7,
    BOOT_MEM_BAD_MEMORY = 8
};

struct boot_mem_region {
    uint64_t base;
    uint64_t length;
    uint32_t type;
};

struct pmm_state {
    uint8_t *bitmap;
    uint64_t bitmap_bytes;
    uint64_t max_phys;
    uint64_t frame_count;
    uint64_t free_frames;
    uint64_t used_frames;
    uint64_t reserved_frames;
    uint64_t ignored_frames;
    uint64_t next_hint;
    bool initialized;
};

void pmm_zero_state(struct pmm_state *pmm);
bool pmm_init_from_map(struct pmm_state *pmm,
                       const struct boot_mem_region *regions,
                       size_t region_count,
                       uint8_t *bitmap_storage,
                       uint64_t bitmap_storage_bytes,
                       uint64_t max_phys_bytes);
uint64_t pmm_alloc_frame(struct pmm_state *pmm);
bool pmm_free_frame(struct pmm_state *pmm, uint64_t phys_addr);
bool pmm_reserve_range(struct pmm_state *pmm, uint64_t base, uint64_t length);
bool pmm_is_frame_free(const struct pmm_state *pmm, uint64_t phys_addr);
uint64_t pmm_free_count(const struct pmm_state *pmm);
uint64_t pmm_used_count(const struct pmm_state *pmm);
uint64_t pmm_frame_count(const struct pmm_state *pmm);

#endif
