#include "mcsos/block.h"

static void mcsos_memcpy_u8_bcache(void *dst, const void *src, uint64_t n) {
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    for (uint64_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
}

static mcsos_bcache_entry_t *mcsos_bcache_find(mcsos_bcache_t *cache, mcsos_blk_device_t *dev, uint64_t lba) {
    for (uint32_t i = 0; i < cache->entry_count; i++) {
        mcsos_bcache_entry_t *e = &cache->entries[i];
        if (e->valid && e->dev == dev && e->lba == lba) {
            return e;
        }
    }
    return 0;
}

static mcsos_blk_status_t mcsos_bcache_flush_entry(mcsos_bcache_entry_t *e) {
    if (e == 0 || !e->valid || !e->dirty) {
        return MCSOS_BLK_OK;
    }
    mcsos_blk_status_t st = mcsos_blk_write(e->dev, e->lba, 1u, e->data);
    if (st != MCSOS_BLK_OK) {
        return st;
    }
    e->dirty = 0;
    return MCSOS_BLK_OK;
}

static mcsos_blk_status_t mcsos_bcache_select_victim(mcsos_bcache_t *cache, mcsos_bcache_entry_t **out) {
    if (cache == 0 || out == 0 || cache->entry_count == 0u) {
        return MCSOS_BLK_EINVAL;
    }
    uint32_t start = (uint32_t)(cache->clock_hand % cache->entry_count);
    for (uint32_t pass = 0; pass < cache->entry_count; pass++) {
        uint32_t idx = (start + pass) % cache->entry_count;
        if (!cache->entries[idx].valid) {
            cache->clock_hand = idx + 1u;
            *out = &cache->entries[idx];
            return MCSOS_BLK_OK;
        }
    }
    uint32_t idx = start;
    cache->clock_hand = idx + 1u;
    mcsos_blk_status_t st = mcsos_bcache_flush_entry(&cache->entries[idx]);
    if (st != MCSOS_BLK_OK) {
        return st;
    }
    *out = &cache->entries[idx];
    return MCSOS_BLK_OK;
}

mcsos_blk_status_t mcsos_bcache_init(mcsos_bcache_t *cache,
                                     mcsos_bcache_entry_t *entries,
                                     uint32_t entry_count,
                                     uint8_t *data_pool,
                                     uint32_t block_size) {
    if (cache == 0 || entries == 0 || data_pool == 0 || entry_count == 0u || block_size == 0u) {
        return MCSOS_BLK_EINVAL;
    }
    cache->entries = entries;
    cache->entry_count = entry_count;
    cache->data_pool = data_pool;
    cache->block_size = block_size;
    cache->clock_hand = 0;
    for (uint32_t i = 0; i < entry_count; i++) {
        entries[i].data = data_pool + ((uint64_t)i * (uint64_t)block_size);
        entries[i].capacity = block_size;
        entries[i].lba = 0;
        entries[i].valid = 0;
        entries[i].dirty = 0;
        entries[i].dev = 0;
    }
    return MCSOS_BLK_OK;
}

mcsos_blk_status_t mcsos_bcache_read(mcsos_bcache_t *cache,
                                     mcsos_blk_device_t *dev,
                                     uint64_t lba,
                                     void *buffer) {
    if (cache == 0 || dev == 0 || buffer == 0 || cache->block_size != dev->block_size) {
        return MCSOS_BLK_EINVAL;
    }
    mcsos_bcache_entry_t *e = mcsos_bcache_find(cache, dev, lba);
    if (e == 0) {
        mcsos_blk_status_t st = mcsos_bcache_select_victim(cache, &e);
        if (st != MCSOS_BLK_OK) {
            return st;
        }
        st = mcsos_blk_read(dev, lba, 1u, e->data);
        if (st != MCSOS_BLK_OK) {
            e->valid = 0;
            return st;
        }
        e->dev = dev;
        e->lba = lba;
        e->valid = 1;
        e->dirty = 0;
    }
    mcsos_memcpy_u8_bcache(buffer, e->data, cache->block_size);
    return MCSOS_BLK_OK;
}

mcsos_blk_status_t mcsos_bcache_write(mcsos_bcache_t *cache,
                                      mcsos_blk_device_t *dev,
                                      uint64_t lba,
                                      const void *buffer) {
    if (cache == 0 || dev == 0 || buffer == 0 || cache->block_size != dev->block_size) {
        return MCSOS_BLK_EINVAL;
    }
    mcsos_bcache_entry_t *e = mcsos_bcache_find(cache, dev, lba);
    if (e == 0) {
        mcsos_blk_status_t st = mcsos_bcache_select_victim(cache, &e);
        if (st != MCSOS_BLK_OK) {
            return st;
        }
        e->dev = dev;
        e->lba = lba;
        e->valid = 1;
        e->dirty = 0;
    }
    mcsos_memcpy_u8_bcache(e->data, buffer, cache->block_size);
    e->dirty = 1;
    return MCSOS_BLK_OK;
}

mcsos_blk_status_t mcsos_bcache_flush_all(mcsos_bcache_t *cache) {
    if (cache == 0 || cache->entries == 0) {
        return MCSOS_BLK_EINVAL;
    }
    for (uint32_t i = 0; i < cache->entry_count; i++) {
        mcsos_blk_status_t st = mcsos_bcache_flush_entry(&cache->entries[i]);
        if (st != MCSOS_BLK_OK) {
            return st;
        }
    }
    return MCSOS_BLK_OK;
}
