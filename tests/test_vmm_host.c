#include "vmm.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define TEST_FRAMES 64U
static unsigned char phys[TEST_FRAMES][VMM_PAGE_SIZE];
static bool used[TEST_FRAMES];

static void *host_phys_to_virt(void *ctx, uint64_t paddr) {
    (void)ctx;
    if (!vmm_is_aligned_4k(paddr)) { return 0; }
    uint64_t frame = paddr / VMM_PAGE_SIZE;
    if (frame >= TEST_FRAMES) { return 0; }
    return phys[frame];
}

static uint64_t host_alloc(void *ctx) {
    (void)ctx;
    for (uint64_t i = 2; i < TEST_FRAMES; i++) {
        if (!used[i]) {
            used[i] = true;
            memset(phys[i], 0xA5, VMM_PAGE_SIZE);
            return i * VMM_PAGE_SIZE;
        }
    }
    return VMM_INVALID_PHYS;
}

static void host_free(void *ctx, uint64_t paddr) {
    (void)ctx;
    assert(vmm_is_aligned_4k(paddr));
    uint64_t frame = paddr / VMM_PAGE_SIZE;
    assert(frame < TEST_FRAMES);
    used[frame] = false;
}

int main(void) {
    memset(phys, 0, sizeof(phys));
    memset(used, 0, sizeof(used));
    used[1] = true;

    struct vmm_space space;
    assert(vmm_space_init(&space, VMM_PAGE_SIZE, 0, host_alloc, host_free, host_phys_to_virt) == VMM_MAP_OK);
    assert(vmm_is_canonical(0xFFFF800000200000ULL));
    assert(!vmm_is_canonical(0x0000800000000000ULL));
    assert(vmm_map_page(&space, 0xFFFF800000200000ULL, 0x0000000000300000ULL,
                        VMM_PTE_WRITABLE | VMM_PTE_GLOBAL | VMM_PTE_NO_EXECUTE) == VMM_MAP_OK);

    struct vmm_mapping m;
    assert(vmm_query_page(&space, 0xFFFF800000200000ULL, &m) == VMM_MAP_OK);
    assert(m.vaddr == 0xFFFF800000200000ULL);
    assert(m.paddr == 0x0000000000300000ULL);
    assert((m.flags & VMM_PTE_PRESENT) != 0);
    assert((m.flags & VMM_PTE_WRITABLE) != 0);
    assert((m.flags & VMM_PTE_NO_EXECUTE) != 0);

    assert(vmm_map_page(&space, 0xFFFF800000200000ULL, 0x0000000000400000ULL, 0) == VMM_ERR_EXISTS);
    assert(vmm_map_page(&space, 0xFFFF800000201000ULL, 0x0000000000400001ULL, 0) == VMM_ERR_INVAL);
    assert(vmm_map_page(&space, 0x0000800000000000ULL, 0x0000000000400000ULL, 0) == VMM_ERR_INVAL);
    assert(vmm_unmap_page(&space, 0xFFFF800000200000ULL) == VMM_MAP_OK);
    assert(vmm_query_page(&space, 0xFFFF800000200000ULL, &m) == VMM_ERR_NOT_FOUND);
    assert(vmm_unmap_page(&space, 0xFFFF800000200000ULL) == VMM_ERR_NOT_FOUND);

    assert(vmm_map_page(&space, 0x0000000000400000ULL, 0x0000000000500000ULL, VMM_PTE_WRITABLE) == VMM_MAP_OK);
    assert(vmm_query_page(&space, 0x0000000000400000ULL, &m) == VMM_MAP_OK);
    assert(m.paddr == 0x0000000000500000ULL);

    puts("M7 VMM host tests PASS");
    return 0;
}
