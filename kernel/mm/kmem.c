#include "mcsos/kmem.h"

#define KMEM_MIN_SPLIT 32u

typedef struct kmem_block {
    uint64_t magic;
    size_t   size;
    int      free;
    uint32_t reserved;
    uint64_t reserved2;
    struct kmem_block *prev;
    struct kmem_block *next;
} kmem_block_t;

static unsigned char  *g_heap_base;
static unsigned char  *g_heap_end;
static kmem_block_t   *g_head;
static int             g_initialized;

static size_t kmem_align_up_size(size_t value, size_t align) {
    if (align == 0u) return value;
    const size_t mask = align - 1u;
    if ((align & mask) != 0u) return 0u;
    if (value > (SIZE_MAX - mask)) return 0u;
    return (value + mask) & ~mask;
}

static uintptr_t kmem_align_up_ptr(uintptr_t value, uintptr_t align) {
    const uintptr_t mask = align - 1u;
    if ((align & mask) != 0u) return 0u;
    if (value > (UINTPTR_MAX - mask)) return 0u;
    return (value + mask) & ~mask;
}

static void *kmem_memset(void *dst, int value, size_t bytes) {
    unsigned char *p = (unsigned char *)dst;
    while (bytes-- > 0u) *p++ = (unsigned char)value;
    return dst;
}

static unsigned char *kmem_payload(kmem_block_t *block) {
    return ((unsigned char *)block) + sizeof(kmem_block_t);
}

static kmem_block_t *kmem_header_from_payload(void *ptr) {
    return (kmem_block_t *)(((unsigned char *)ptr) - sizeof(kmem_block_t));
}

static int kmem_ptr_in_heap(const void *ptr) {
    const unsigned char *p = (const unsigned char *)ptr;
    return g_initialized && p >= g_heap_base && p < g_heap_end;
}

static void kmem_split_if_useful(kmem_block_t *block, size_t wanted) {
    const size_t header = kmem_align_up_size(sizeof(kmem_block_t), KMEM_ALIGN);
    if (header == 0u) return;
    if (block->size < wanted + header + KMEM_MIN_SPLIT) return;

    unsigned char *new_addr = kmem_payload(block) + wanted;
    new_addr = (unsigned char *)kmem_align_up_ptr((uintptr_t)new_addr, KMEM_ALIGN);
    if (new_addr == (unsigned char *)0) return;
    if (new_addr + sizeof(kmem_block_t) >= g_heap_end) return;

    const size_t consumed = (size_t)(new_addr - kmem_payload(block));
    if (block->size <= consumed + sizeof(kmem_block_t) + KMEM_MIN_SPLIT) return;

    kmem_block_t *new_block = (kmem_block_t *)new_addr;
    new_block->magic    = KMEM_MAGIC;
    new_block->size     = block->size - consumed - sizeof(kmem_block_t);
    new_block->free     = 1;
    new_block->prev     = block;
    new_block->next     = block->next;
    if (block->next != (kmem_block_t *)0)
        block->next->prev = new_block;
    block->next = new_block;
    block->size = wanted;
}

static void kmem_coalesce_forward(kmem_block_t *block) {
    while (block != (kmem_block_t *)0 &&
           block->next != (kmem_block_t *)0 &&
           block->next->free) {
        kmem_block_t *next = block->next;
        unsigned char *expected = kmem_payload(block) + block->size;
        expected = (unsigned char *)kmem_align_up_ptr((uintptr_t)expected, KMEM_ALIGN);
        if (expected != (unsigned char *)next) return;
        block->size += sizeof(kmem_block_t) + next->size;
        block->next  = next->next;
        if (next->next != (kmem_block_t *)0)
            next->next->prev = block;
        next->magic = 0u;
        next->size  = 0u;
        next->prev  = (kmem_block_t *)0;
        next->next  = (kmem_block_t *)0;
    }
}

int kmem_init(void *base, size_t bytes) {
    if (base == (void *)0 || bytes < (sizeof(kmem_block_t) + KMEM_MIN_SPLIT))
        return -1;

    uintptr_t start = kmem_align_up_ptr((uintptr_t)base, KMEM_ALIGN);
    if (start == 0u || start < (uintptr_t)base) return -2;
    const size_t lost = (size_t)(start - (uintptr_t)base);
    if (bytes <= lost + sizeof(kmem_block_t) + KMEM_MIN_SPLIT) return -3;

    size_t usable = bytes - lost;
    usable = usable & ~(size_t)(KMEM_ALIGN - 1u);
    if (usable <= sizeof(kmem_block_t) + KMEM_MIN_SPLIT) return -4;

    g_heap_base = (unsigned char *)start;
    g_heap_end  = g_heap_base + usable;
    g_head      = (kmem_block_t *)g_heap_base;
    g_head->magic    = KMEM_MAGIC;
    g_head->size     = usable - sizeof(kmem_block_t);
    g_head->free     = 1;
    g_head->prev     = (kmem_block_t *)0;
    g_head->next     = (kmem_block_t *)0;
    g_initialized    = 1;
    return kmem_validate();
}

void *kmem_alloc(size_t bytes) {
    if (!g_initialized || bytes == 0u) return (void *)0;
    const size_t wanted = kmem_align_up_size(bytes, KMEM_ALIGN);
    if (wanted == 0u) return (void *)0;

    for (kmem_block_t *cur = g_head; cur != (kmem_block_t *)0; cur = cur->next) {
        if (cur->magic != KMEM_MAGIC) return (void *)0;
        if (cur->free && cur->size >= wanted) {
            kmem_split_if_useful(cur, wanted);
            cur->free = 0;
            return (void *)kmem_payload(cur);
        }
    }
    return (void *)0;
}

void *kmem_calloc(size_t count, size_t bytes) {
    if (count != 0u && bytes > SIZE_MAX / count) return (void *)0;
    const size_t total = count * bytes;
    void *ptr = kmem_alloc(total);
    if (ptr != (void *)0) (void)kmem_memset(ptr, 0, total);
    return ptr;
}

int kmem_free_checked(void *ptr) {
    if (ptr == (void *)0) return 0;
    if (!kmem_ptr_in_heap(ptr)) return -1;
    if (((uintptr_t)ptr & (KMEM_ALIGN - 1u)) != 0u) return -2;
    kmem_block_t *block = kmem_header_from_payload(ptr);
    if (!kmem_ptr_in_heap(block) || block->magic != KMEM_MAGIC) return -3;
    if (block->free) return -4;
    block->free = 1;
    kmem_coalesce_forward(block);
    if (block->prev != (kmem_block_t *)0 && block->prev->free)
        kmem_coalesce_forward(block->prev);
    return kmem_validate();
}

void kmem_get_stats(kmem_stats_t *out) {
    if (out == (kmem_stats_t *)0) return;
    out->total_bytes  = 0u;
    out->used_bytes   = 0u;
    out->free_bytes   = 0u;
    out->block_count  = 0u;
    out->free_count   = 0u;
    out->largest_free = 0u;
    if (!g_initialized) return;
    out->total_bytes = (size_t)(g_heap_end - g_heap_base);
    for (kmem_block_t *cur = g_head; cur != (kmem_block_t *)0; cur = cur->next) {
        out->block_count++;
        if (cur->free) {
            out->free_count++;
            out->free_bytes += cur->size;
            if (cur->size > out->largest_free) out->largest_free = cur->size;
        } else {
            out->used_bytes += cur->size;
        }
    }
}

int kmem_validate(void) {
    if (!g_initialized || g_heap_base == (unsigned char *)0 ||
        g_heap_end <= g_heap_base || g_head == (kmem_block_t *)0)
        return -1;
    if ((unsigned char *)g_head != g_heap_base) return -2;

    kmem_block_t  *prev   = (kmem_block_t *)0;
    unsigned char *cursor = g_heap_base;
    size_t guard = 0u;
    for (kmem_block_t *cur = g_head; cur != (kmem_block_t *)0; cur = cur->next) {
        if (++guard > 1048576u) return -3;
        if ((unsigned char *)cur != cursor) return -4;
        if ((unsigned char *)cur < g_heap_base ||
            ((unsigned char *)cur + sizeof(kmem_block_t)) > g_heap_end) return -5;
        if (cur->magic != KMEM_MAGIC) return -6;
        if (cur->prev != prev) return -7;
        if (cur->size > (size_t)(g_heap_end - kmem_payload(cur))) return -8;
        cursor = kmem_payload(cur) + cur->size;
        cursor = (unsigned char *)kmem_align_up_ptr((uintptr_t)cursor, KMEM_ALIGN);
        if (cursor == (unsigned char *)0 || cursor > g_heap_end) return -9;
        prev = cur;
    }
    return 0;
}
