#include <assert.h>
#include <stdio.h>
#include "pmm.h"

static uint8_t bitmap[PMM_BITMAP_BYTES];

int main(void) {
    struct boot_mem_region regions[] = {
        { .base = 0x00000000ULL, .length = 0x0009f000ULL, .type = BOOT_MEM_USABLE },
        { .base = 0x0009f000ULL, .length = 0x00001000ULL, .type = BOOT_MEM_RESERVED },
        { .base = 0x00100000ULL, .length = 0x00300000ULL, .type = BOOT_MEM_USABLE },
        { .base = 0x00400000ULL, .length = 0x00100000ULL, .type = BOOT_MEM_KERNEL_AND_MODULES },
        { .base = 0x00500000ULL, .length = 0x00400000ULL, .type = BOOT_MEM_USABLE },
    };

    struct pmm_state pmm;
    assert(pmm_init_from_map(&pmm, regions, sizeof(regions) / sizeof(regions[0]),
                             bitmap, sizeof(bitmap), 64ULL * 1024ULL * 1024ULL));
    assert(pmm_frame_count(&pmm) == (64ULL * 1024ULL * 1024ULL) / PMM_PAGE_SIZE);
    assert(!pmm_is_frame_free(&pmm, 0));
    assert(pmm_is_frame_free(&pmm, 0x00100000ULL));
    assert(!pmm_is_frame_free(&pmm, 0x00400000ULL));

    uint64_t before = pmm_free_count(&pmm);
    uint64_t frame = pmm_alloc_frame(&pmm);
    assert(frame != PMM_INVALID_FRAME);
    assert((frame & (PMM_PAGE_SIZE - 1ULL)) == 0);
    assert(!pmm_is_frame_free(&pmm, frame));
    assert(pmm_free_count(&pmm) == before - 1ULL);
    assert(pmm_free_frame(&pmm, frame));
    assert(pmm_free_count(&pmm) == before);
    assert(!pmm_free_frame(&pmm, frame));

    assert(pmm_reserve_range(&pmm, 0x00500000ULL, 0x2000ULL));
    assert(!pmm_is_frame_free(&pmm, 0x00500000ULL));
    assert(!pmm_is_frame_free(&pmm, 0x00501000ULL));

    puts("M6 PMM host unit test: PASS");
    return 0;
}
