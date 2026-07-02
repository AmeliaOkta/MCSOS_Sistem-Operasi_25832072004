#include "mcsos/block.h"

extern void mcsos_blk_copy_name_for_driver(char dst[MCSOS_BLK_NAME_MAX], const char *src);

static void mcsos_memcpy_u8(void *dst, const void *src, uint64_t n) {
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    for (uint64_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
}

static int mcsos_is_power_of_two_u32_local(uint32_t value) {
    return value != 0u && (value & (value - 1u)) == 0u;
}

static mcsos_blk_status_t mcsos_ramblk_rw(mcsos_blk_device_t *dev, uint64_t lba, uint32_t count, void *buffer, int is_write) {
    if (dev == 0 || dev->driver_data == 0 || buffer == 0) {
        return MCSOS_BLK_EINVAL;
    }
    mcsos_ramblk_t *ram = (mcsos_ramblk_t *)dev->driver_data;
    uint64_t byte_offset = lba * (uint64_t)dev->block_size;
    uint64_t byte_count = (uint64_t)count * (uint64_t)dev->block_size;
    if (byte_offset > ram->storage_size || byte_count > ram->storage_size - byte_offset) {
        return MCSOS_BLK_ERANGE;
    }
    if (is_write) {
        mcsos_memcpy_u8(ram->storage + byte_offset, buffer, byte_count);
    } else {
        mcsos_memcpy_u8(buffer, ram->storage + byte_offset, byte_count);
    }
    return MCSOS_BLK_OK;
}

static mcsos_blk_status_t mcsos_ramblk_read(mcsos_blk_device_t *dev, uint64_t lba, uint32_t count, void *buffer) {
    return mcsos_ramblk_rw(dev, lba, count, buffer, 0);
}

static mcsos_blk_status_t mcsos_ramblk_write(mcsos_blk_device_t *dev, uint64_t lba, uint32_t count, void *buffer) {
    return mcsos_ramblk_rw(dev, lba, count, buffer, 1);
}

static mcsos_blk_status_t mcsos_ramblk_flush(mcsos_blk_device_t *dev, uint64_t lba, uint32_t count, void *buffer) {
    (void)dev;
    (void)lba;
    (void)count;
    (void)buffer;
    return MCSOS_BLK_OK;
}

static const mcsos_blk_ops_t g_ramblk_ops = {
    .read = mcsos_ramblk_read,
    .write = mcsos_ramblk_write,
    .flush = mcsos_ramblk_flush,
};

mcsos_blk_status_t mcsos_ramblk_init(mcsos_blk_device_t *dev,
                                     mcsos_ramblk_t *ram,
                                     const char *name,
                                     uint8_t *storage,
                                     uint64_t storage_size,
                                     uint32_t block_size) {
    if (dev == 0 || ram == 0 || storage == 0 || name == 0) {
        return MCSOS_BLK_EINVAL;
    }
    if (block_size < MCSOS_BLK_DEFAULT_SECTOR_SIZE || !mcsos_is_power_of_two_u32_local(block_size)) {
        return MCSOS_BLK_EINVAL;
    }
    if (storage_size < block_size || (storage_size % block_size) != 0u) {
        return MCSOS_BLK_EINVAL;
    }
    ram->storage = storage;
    ram->storage_size = storage_size;
    mcsos_blk_copy_name_for_driver(dev->name, name);
    dev->block_size = block_size;
    dev->block_count = storage_size / block_size;
    dev->flags = 0;
    dev->ops = &g_ramblk_ops;
    dev->driver_data = ram;
    return MCSOS_BLK_OK;
}
