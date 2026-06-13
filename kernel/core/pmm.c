#include "pmm.h"

#ifndef UINT64_MAX
#define UINT64_MAX 0xffffffffffffffffULL
#endif

static uint64_t align_down(uint64_t value, uint64_t align) {
    return value & ~(align - 1ULL);
}

static uint64_t align_up(uint64_t value, uint64_t align) {
    return (value + align - 1ULL) & ~(align - 1ULL);
}

static bool checked_add_u64(uint64_t a, uint64_t b, uint64_t *out) {
    if (UINT64_MAX - a < b) {
        return false;
    }
    *out = a + b;
    return true;
}

static void bitmap_set(uint8_t *bitmap, uint64_t index) {
    bitmap[index >> 3] = (uint8_t)(bitmap[index >> 3] | (uint8_t)(1U << (index & 7U)));
}

static void bitmap_clear(uint8_t *bitmap, uint64_t index) {
    bitmap[index >> 3] = (uint8_t)(bitmap[index >> 3] & (uint8_t)~(uint8_t)(1U << (index & 7U)));
}

static bool bitmap_test(const uint8_t *bitmap, uint64_t index) {
    return (bitmap[index >> 3] & (uint8_t)(1U << (index & 7U))) != 0;
}

static void mark_frame_free(struct pmm_state *pmm, uint64_t frame) {
    if (frame >= pmm->frame_count) {
        return;
    }
    if (bitmap_test(pmm->bitmap, frame)) {
        bitmap_clear(pmm->bitmap, frame);
        pmm->free_frames++;
        if (pmm->used_frames > 0) {
            pmm->used_frames--;
        }
        if (frame < pmm->next_hint) {
            pmm->next_hint = frame;
        }
    }
}

static void mark_frame_used(struct pmm_state *pmm, uint64_t frame) {
    if (frame >= pmm->frame_count) {
        return;
    }
    if (!bitmap_test(pmm->bitmap, frame)) {
        bitmap_set(pmm->bitmap, frame);
        if (pmm->free_frames > 0) {
            pmm->free_frames--;
        }
        pmm->used_frames++;
    }
}

static void mark_range_free(struct pmm_state *pmm, uint64_t base, uint64_t length) {
    uint64_t end;
    if (length == 0 || !checked_add_u64(base, length, &end)) {
        return;
    }
    uint64_t start = align_up(base, PMM_PAGE_SIZE);
    uint64_t stop = align_down(end, PMM_PAGE_SIZE);
    if (stop <= start) {
        return;
    }
    if (start >= pmm->max_phys) {
        return;
    }
    if (stop > pmm->max_phys) {
        stop = pmm->max_phys;
        pmm->ignored_frames += (align_down(end, PMM_PAGE_SIZE) - stop) / PMM_PAGE_SIZE;
    }
    for (uint64_t addr = start; addr < stop; addr += PMM_PAGE_SIZE) {
        mark_frame_free(pmm, addr / PMM_PAGE_SIZE);
    }
}

static void mark_range_used(struct pmm_state *pmm, uint64_t base, uint64_t length) {
    uint64_t end;
    if (length == 0 || !checked_add_u64(base, length, &end)) {
        return;
    }
    uint64_t start = align_down(base, PMM_PAGE_SIZE);
    uint64_t stop = align_up(end, PMM_PAGE_SIZE);
    if (stop <= start) {
        return;
    }
    if (start >= pmm->max_phys) {
        return;
    }
    if (stop > pmm->max_phys) {
        stop = pmm->max_phys;
    }
    for (uint64_t addr = start; addr < stop; addr += PMM_PAGE_SIZE) {
        bool was_free = !bitmap_test(pmm->bitmap, addr / PMM_PAGE_SIZE);
        mark_frame_used(pmm, addr / PMM_PAGE_SIZE);
        if (was_free) {
            pmm->reserved_frames++;
        }
    }
}

void pmm_zero_state(struct pmm_state *pmm) {
    if (pmm == NULL) {
        return;
    }
    pmm->bitmap = NULL;
    pmm->bitmap_bytes = 0;
    pmm->max_phys = 0;
    pmm->frame_count = 0;
    pmm->free_frames = 0;
    pmm->used_frames = 0;
    pmm->reserved_frames = 0;
    pmm->ignored_frames = 0;
    pmm->next_hint = 0;
    pmm->initialized = false;
}

bool pmm_init_from_map(struct pmm_state *pmm,
                       const struct boot_mem_region *regions,
                       size_t region_count,
                       uint8_t *bitmap_storage,
                       uint64_t bitmap_storage_bytes,
                       uint64_t max_phys_bytes) {
    if (pmm == NULL || regions == NULL || bitmap_storage == NULL || region_count == 0) {
        return false;
    }
    if (max_phys_bytes == 0 || (max_phys_bytes & (PMM_PAGE_SIZE - 1ULL)) != 0) {
        return false;
    }
    uint64_t frame_count = max_phys_bytes / PMM_PAGE_SIZE;
    uint64_t required_bitmap_bytes = (frame_count + 7ULL) / 8ULL;
    if (bitmap_storage_bytes < required_bitmap_bytes) {
        return false;
    }

    pmm_zero_state(pmm);
    pmm->bitmap = bitmap_storage;
    pmm->bitmap_bytes = required_bitmap_bytes;
    pmm->max_phys = max_phys_bytes;
    pmm->frame_count = frame_count;
    pmm->free_frames = 0;
    pmm->used_frames = frame_count;
    pmm->next_hint = 0;

    for (uint64_t i = 0; i < required_bitmap_bytes; i++) {
        bitmap_storage[i] = 0xffU;
    }

    for (size_t i = 0; i < region_count; i++) {
        if (regions[i].type == BOOT_MEM_USABLE) {
            mark_range_free(pmm, regions[i].base, regions[i].length);
        }
    }

    /* Frame 0 is never allocated in MCSOS. It catches null-like physical addresses. */
    mark_range_used(pmm, 0, PMM_PAGE_SIZE);

    for (size_t i = 0; i < region_count; i++) {
        if (regions[i].type != BOOT_MEM_USABLE) {
            mark_range_used(pmm, regions[i].base, regions[i].length);
        }
    }

    pmm->initialized = true;
    return true;
}

uint64_t pmm_alloc_frame(struct pmm_state *pmm) {
    if (pmm == NULL || !pmm->initialized || pmm->free_frames == 0) {
        return PMM_INVALID_FRAME;
    }
    for (uint64_t frame = pmm->next_hint; frame < pmm->frame_count; frame++) {
        if (!bitmap_test(pmm->bitmap, frame)) {
            mark_frame_used(pmm, frame);
            pmm->next_hint = frame + 1ULL;
            return frame * PMM_PAGE_SIZE;
        }
    }
    for (uint64_t frame = 0; frame < pmm->next_hint; frame++) {
        if (!bitmap_test(pmm->bitmap, frame)) {
            mark_frame_used(pmm, frame);
            pmm->next_hint = frame + 1ULL;
            return frame * PMM_PAGE_SIZE;
        }
    }
    return PMM_INVALID_FRAME;
}

bool pmm_free_frame(struct pmm_state *pmm, uint64_t phys_addr) {
    if (pmm == NULL || !pmm->initialized) {
        return false;
    }
    if ((phys_addr & (PMM_PAGE_SIZE - 1ULL)) != 0 || phys_addr == 0 || phys_addr >= pmm->max_phys) {
        return false;
    }
    uint64_t frame = phys_addr / PMM_PAGE_SIZE;
    if (!bitmap_test(pmm->bitmap, frame)) {
        return false;
    }
    mark_frame_free(pmm, frame);
    return true;
}

bool pmm_reserve_range(struct pmm_state *pmm, uint64_t base, uint64_t length) {
    if (pmm == NULL || !pmm->initialized || length == 0) {
        return false;
    }
    mark_range_used(pmm, base, length);
    return true;
}

bool pmm_is_frame_free(const struct pmm_state *pmm, uint64_t phys_addr) {
    if (pmm == NULL || !pmm->initialized) {
        return false;
    }
    if ((phys_addr & (PMM_PAGE_SIZE - 1ULL)) != 0 || phys_addr >= pmm->max_phys) {
        return false;
    }
    return !bitmap_test(pmm->bitmap, phys_addr / PMM_PAGE_SIZE);
}

uint64_t pmm_free_count(const struct pmm_state *pmm) {
    return (pmm != NULL) ? pmm->free_frames : 0ULL;
}

uint64_t pmm_used_count(const struct pmm_state *pmm) {
    return (pmm != NULL) ? pmm->used_frames : 0ULL;
}

uint64_t pmm_frame_count(const struct pmm_state *pmm) {
    return (pmm != NULL) ? pmm->frame_count : 0ULL;
}
