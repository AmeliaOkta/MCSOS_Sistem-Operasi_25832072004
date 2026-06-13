#ifndef MCSOS_VMM_H
#define MCSOS_VMM_H

#include "types.h"

#define VMM_PAGE_SIZE 4096ULL
#define VMM_ENTRIES_PER_TABLE 512U
#define VMM_INVALID_PHYS 0xffffffffffffffffULL

#define VMM_PTE_PRESENT        (1ULL << 0)
#define VMM_PTE_WRITABLE       (1ULL << 1)
#define VMM_PTE_USER           (1ULL << 2)
#define VMM_PTE_WRITE_THROUGH  (1ULL << 3)
#define VMM_PTE_CACHE_DISABLE  (1ULL << 4)
#define VMM_PTE_ACCESSED       (1ULL << 5)
#define VMM_PTE_DIRTY          (1ULL << 6)
#define VMM_PTE_HUGE           (1ULL << 7)
#define VMM_PTE_GLOBAL         (1ULL << 8)
#define VMM_PTE_NO_EXECUTE     (1ULL << 63)
#define VMM_PTE_ADDR_MASK      0x000ffffffffff000ULL

#define VMM_MAP_OK         0
#define VMM_ERR_INVAL     -1
#define VMM_ERR_NOMEM     -2
#define VMM_ERR_EXISTS    -3
#define VMM_ERR_NOT_FOUND -4

typedef uint64_t (*vmm_alloc_frame_fn)(void *ctx);
typedef void (*vmm_free_frame_fn)(void *ctx, uint64_t frame_paddr);
typedef void *(*vmm_phys_to_virt_fn)(void *ctx, uint64_t paddr);

struct vmm_space {
    uint64_t root_paddr;
    void *ctx;
    vmm_alloc_frame_fn alloc_frame;
    vmm_free_frame_fn free_frame;
    vmm_phys_to_virt_fn phys_to_virt;
};

struct vmm_mapping {
    uint64_t vaddr;
    uint64_t paddr;
    uint64_t flags;
};

bool vmm_is_aligned_4k(uint64_t value);
bool vmm_is_canonical(uint64_t vaddr);

int vmm_space_init(struct vmm_space *space,
                   uint64_t root_paddr,
                   void *ctx,
                   vmm_alloc_frame_fn alloc_frame,
                   vmm_free_frame_fn free_frame,
                   vmm_phys_to_virt_fn phys_to_virt);

int vmm_map_page(struct vmm_space *space, uint64_t vaddr, uint64_t paddr, uint64_t flags);
int vmm_unmap_page(struct vmm_space *space, uint64_t vaddr);
int vmm_query_page(struct vmm_space *space, uint64_t vaddr, struct vmm_mapping *out);

void vmm_invalidate_page(uint64_t vaddr);
uint64_t vmm_read_cr3(void);
void vmm_write_cr3(uint64_t value);
uint64_t vmm_read_cr2(void);

#endif
