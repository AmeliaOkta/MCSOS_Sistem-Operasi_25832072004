#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "mcsos/block.h"

#define EXPECT_OK(x) do { mcsos_blk_status_t st__ = (x); if (st__ != MCSOS_BLK_OK) { printf("FAIL:%s:%d status=%d\n", __FILE__, __LINE__, (int)st__); return 1; } } while (0)
#define EXPECT_EQ(a,b) do { uint64_t aa__=(uint64_t)(a); uint64_t bb__=(uint64_t)(b); if (aa__ != bb__) { printf("FAIL:%s:%d got=%llu want=%llu\n", __FILE__, __LINE__, (unsigned long long)aa__, (unsigned long long)bb__); return 1; } } while (0)
#define EXPECT_STATUS(expr,want) do { mcsos_blk_status_t st__=(expr); if (st__ != (want)) { printf("FAIL:%s:%d status=%d want=%d\n", __FILE__, __LINE__, (int)st__, (int)(want)); return 1; } } while (0)

static void fill(uint8_t *p, size_t n, uint8_t seed) {
    for (size_t i = 0; i < n; i++) {
        p[i] = (uint8_t)(seed + (uint8_t)i);
    }
}

int main(void) {
    uint8_t backing[512u * 32u];
    uint8_t tmp[512u];
    uint8_t out[512u];
    memset(backing, 0, sizeof(backing));
    memset(tmp, 0, sizeof(tmp));
    memset(out, 0, sizeof(out));

    mcsos_blk_registry_reset();
    mcsos_blk_device_t dev;
    mcsos_ramblk_t ram;
    EXPECT_OK(mcsos_ramblk_init(&dev, &ram, "ram0", backing, sizeof(backing), 512u));
    EXPECT_OK(mcsos_blk_register(&dev));
    EXPECT_EQ(mcsos_blk_count(), 1u);
    EXPECT_EQ(mcsos_blk_get(0u), &dev);
    EXPECT_EQ(dev.block_count, 32u);

    fill(tmp, sizeof(tmp), 7u);
    EXPECT_OK(mcsos_blk_write(&dev, 3u, 1u, tmp));
    EXPECT_OK(mcsos_blk_read(&dev, 3u, 1u, out));
    EXPECT_EQ(memcmp(tmp, out, sizeof(tmp)), 0);
    EXPECT_STATUS(mcsos_blk_read(&dev, 32u, 1u, out), MCSOS_BLK_ERANGE);
    EXPECT_STATUS(mcsos_blk_write(&dev, 31u, 2u, tmp), MCSOS_BLK_ERANGE);
    EXPECT_STATUS(mcsos_blk_write(&dev, 0u, 0u, tmp), MCSOS_BLK_EINVAL);
    EXPECT_STATUS(mcsos_blk_write(&dev, 0u, 1u, 0), MCSOS_BLK_EINVAL);

    mcsos_bcache_t cache;
    mcsos_bcache_entry_t entries[2];
    uint8_t pool[2u * 512u];
    EXPECT_OK(mcsos_bcache_init(&cache, entries, 2u, pool, 512u));
    fill(tmp, sizeof(tmp), 42u);
    EXPECT_OK(mcsos_bcache_write(&cache, &dev, 4u, tmp));
    memset(out, 0, sizeof(out));
    EXPECT_OK(mcsos_bcache_read(&cache, &dev, 4u, out));
    EXPECT_EQ(memcmp(tmp, out, sizeof(tmp)), 0);
    memset(out, 0, sizeof(out));
    EXPECT_OK(mcsos_blk_read(&dev, 4u, 1u, out));
    EXPECT_EQ(memcmp(tmp, out, sizeof(tmp)) != 0, 1u);
    EXPECT_OK(mcsos_bcache_flush_all(&cache));
    EXPECT_OK(mcsos_blk_read(&dev, 4u, 1u, out));
    EXPECT_EQ(memcmp(tmp, out, sizeof(tmp)), 0);

    fill(tmp, sizeof(tmp), 100u);
    EXPECT_OK(mcsos_bcache_write(&cache, &dev, 5u, tmp));
    EXPECT_OK(mcsos_bcache_flush_all(&cache));
    EXPECT_OK(mcsos_blk_read(&dev, 5u, 1u, out));
    EXPECT_EQ(memcmp(tmp, out, sizeof(tmp)), 0);

    printf("M14 host tests PASS\n");
    return 0;
}
