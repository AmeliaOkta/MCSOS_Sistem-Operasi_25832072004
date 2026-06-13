#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "mcsos/kmem.h"

static unsigned char arena[4096u * 8u];

static void test_basic_alloc_free(void) {
    assert(kmem_init(arena, sizeof(arena)) == 0);
    void *a = kmem_alloc(24);
    void *b = kmem_alloc(128);
    void *c = kmem_alloc(4096);
    assert(a != NULL);
    assert(b != NULL);
    assert(c != NULL);
    assert(((uintptr_t)a & (KMEM_ALIGN - 1u)) == 0u);
    assert(((uintptr_t)b & (KMEM_ALIGN - 1u)) == 0u);
    assert(((uintptr_t)c & (KMEM_ALIGN - 1u)) == 0u);
    memset(a, 0x11, 24);
    memset(b, 0x22, 128);
    memset(c, 0x33, 4096);
    assert(kmem_validate() == 0);
    assert(kmem_free_checked(b) == 0);
    assert(kmem_free_checked(a) == 0);
    assert(kmem_free_checked(c) == 0);
    assert(kmem_validate() == 0);
}

static void test_calloc_and_overflow(void) {
    assert(kmem_init(arena, sizeof(arena)) == 0);
    unsigned char *z = (unsigned char *)kmem_calloc(64, 4);
    assert(z != NULL);
    for (size_t i = 0; i < 256; ++i) assert(z[i] == 0u);
    assert(kmem_calloc((size_t)-1, 2) == NULL);
    assert(kmem_free_checked(z) == 0);
}

static void test_double_free_rejected(void) {
    assert(kmem_init(arena, sizeof(arena)) == 0);
    void *p = kmem_alloc(512);
    assert(p != NULL);
    assert(kmem_free_checked(p) == 0);
    assert(kmem_free_checked(p) < 0);
}

static void test_null_free_ok(void) {
    assert(kmem_init(arena, sizeof(arena)) == 0);
    assert(kmem_free_checked(NULL) == 0);
}

static void test_fragmentation_and_coalesce(void) {
    assert(kmem_init(arena, sizeof(arena)) == 0);
    void *p[16];
    for (size_t i = 0; i < 16; ++i) {
        p[i] = kmem_alloc(256 + i);
        assert(p[i] != NULL);
    }
    for (size_t i = 0; i < 16; i += 2) assert(kmem_free_checked(p[i]) == 0);
    for (size_t i = 1; i < 16; i += 2) assert(kmem_free_checked(p[i]) == 0);
    kmem_stats_t st;
    kmem_get_stats(&st);
    assert(st.free_count == 1u);
    assert(st.block_count == 1u);
    assert(st.largest_free > 4096u);
}

int main(void) {
    test_basic_alloc_free();
    test_calloc_and_overflow();
    test_double_free_rejected();
    test_null_free_ok();
    test_fragmentation_and_coalesce();
    puts("M8 kmem host tests: PASS");
    return 0;
}
