#include "mcsos/block.h"

static mcsos_blk_device_t *g_blk_devices[MCSOS_BLK_MAX_DEVICES];
static uint32_t g_blk_count;

static int mcsos_is_power_of_two_u32(uint32_t value) {
    return value != 0u && (value & (value - 1u)) == 0u;
}

static int mcsos_name_is_nonempty(const char *s) {
    return s != 0 && s[0] != '\0';
}

static void mcsos_copy_name(char dst[MCSOS_BLK_NAME_MAX], const char *src) {
    uint32_t i = 0;
    if (src == 0) {
        dst[0] = '\0';
        return;
    }
    while (i + 1u < MCSOS_BLK_NAME_MAX && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

void mcsos_blk_registry_reset(void) {
    for (uint32_t i = 0; i < MCSOS_BLK_MAX_DEVICES; i++) {
        g_blk_devices[i] = 0;
    }
    g_blk_count = 0;
}

mcsos_blk_status_t mcsos_blk_register(mcsos_blk_device_t *dev) {
    if (dev == 0 || dev->ops == 0 || dev->ops->read == 0 || dev->ops->write == 0) {
        return MCSOS_BLK_EINVAL;
    }
    if (!mcsos_name_is_nonempty(dev->name) || dev->block_count == 0u ||
        dev->block_size < MCSOS_BLK_DEFAULT_SECTOR_SIZE ||
        !mcsos_is_power_of_two_u32(dev->block_size)) {
        return MCSOS_BLK_EINVAL;
    }
    if (g_blk_count >= MCSOS_BLK_MAX_DEVICES) {
        return MCSOS_BLK_EFULL;
    }
    g_blk_devices[g_blk_count++] = dev;
    return MCSOS_BLK_OK;
}

mcsos_blk_device_t *mcsos_blk_get(uint32_t index) {
    if (index >= g_blk_count) {
        return 0;
    }
    return g_blk_devices[index];
}

uint32_t mcsos_blk_count(void) {
    return g_blk_count;
}

static mcsos_blk_status_t mcsos_blk_validate_range(mcsos_blk_device_t *dev, uint64_t lba, uint32_t count, const void *buffer) {
    if (dev == 0 || buffer == 0 || count == 0u || dev->ops == 0) {
        return MCSOS_BLK_EINVAL;
    }
    if (lba >= dev->block_count) {
        return MCSOS_BLK_ERANGE;
    }
    if ((uint64_t)count > dev->block_count - lba) {
        return MCSOS_BLK_ERANGE;
    }
    return MCSOS_BLK_OK;
}

mcsos_blk_status_t mcsos_blk_read(mcsos_blk_device_t *dev, uint64_t lba, uint32_t count, void *buffer) {
    mcsos_blk_status_t st = mcsos_blk_validate_range(dev, lba, count, buffer);
    if (st != MCSOS_BLK_OK) {
        return st;
    }
    if (dev->ops->read == 0) {
        return MCSOS_BLK_EINVAL;
    }
    return dev->ops->read(dev, lba, count, buffer);
}

mcsos_blk_status_t mcsos_blk_write(mcsos_blk_device_t *dev, uint64_t lba, uint32_t count, const void *buffer) {
    mcsos_blk_status_t st = mcsos_blk_validate_range(dev, lba, count, buffer);
    if (st != MCSOS_BLK_OK) {
        return st;
    }
    if (dev->ops->write == 0) {
        return MCSOS_BLK_EINVAL;
    }
    return dev->ops->write(dev, lba, count, (void *)buffer);
}

mcsos_blk_status_t mcsos_blk_flush(mcsos_blk_device_t *dev) {
    if (dev == 0 || dev->ops == 0) {
        return MCSOS_BLK_EINVAL;
    }
    if (dev->ops->flush == 0) {
        return MCSOS_BLK_OK;
    }
    return dev->ops->flush(dev, 0, 0, 0);
}

void mcsos_blk_copy_name_for_driver(char dst[MCSOS_BLK_NAME_MAX], const char *src) {
    mcsos_copy_name(dst, src);
}
