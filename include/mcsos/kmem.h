#ifndef MCSOS_KMEM_H
#define MCSOS_KMEM_H

#include <stddef.h>
#include <stdint.h>

#define KMEM_ALIGN 16u
#define KMEM_MAGIC 0x4d43534f53484541ull

typedef struct kmem_stats {
    size_t total_bytes;
    size_t used_bytes;
    size_t free_bytes;
    size_t block_count;
    size_t free_count;
    size_t largest_free;
} kmem_stats_t;

int   kmem_init(void *base, size_t bytes);
void *kmem_alloc(size_t bytes);
void *kmem_calloc(size_t count, size_t bytes);
int   kmem_free_checked(void *ptr);
void  kmem_get_stats(kmem_stats_t *out);
int   kmem_validate(void);

#endif
