#ifndef MCSOS_BLOCK_H
#define MCSOS_BLOCK_H

#include <stddef.h>
#include <stdint.h>

#define MCSOS_BLK_NAME_MAX 16u
#define MCSOS_BLK_MAX_DEVICES 8u
#define MCSOS_BLK_DEFAULT_SECTOR_SIZE 512u

typedef enum mcsos_blk_status {
    MCSOS_BLK_OK = 0,
    MCSOS_BLK_EINVAL = -1,
    MCSOS_BLK_ERANGE = -2,
    MCSOS_BLK_EFULL = -3,
    MCSOS_BLK_EIO = -4,
    MCSOS_BLK_ENODEV = -5
} mcsos_blk_status_t;

struct mcsos_blk_device;

typedef mcsos_blk_status_t (*mcsos_blk_rw_fn)(struct mcsos_blk_device *dev,
                                               uint64_t lba,
                                               uint32_t count,
                                               void *buffer);

typedef struct mcsos_blk_ops {
    mcsos_blk_rw_fn read;
    mcsos_blk_rw_fn write;
    mcsos_blk_rw_fn flush;
} mcsos_blk_ops_t;

typedef struct mcsos_blk_device {
    char name[MCSOS_BLK_NAME_MAX];
    uint32_t block_size;
    uint64_t block_count;
    uint32_t flags;
    const mcsos_blk_ops_t *ops;
    void *driver_data;
} mcsos_blk_device_t;

typedef struct mcsos_ramblk {
    uint8_t *storage;
    uint64_t storage_size;
} mcsos_ramblk_t;

typedef struct mcsos_bcache_entry {
    uint8_t *data;
    uint32_t capacity;
    uint64_t lba;
    int valid;
    int dirty;
    mcsos_blk_device_t *dev;
} mcsos_bcache_entry_t;

typedef struct mcsos_bcache {
    mcsos_bcache_entry_t *entries;
    uint32_t entry_count;
    uint8_t *data_pool;
    uint32_t block_size;
    uint64_t clock_hand;
} mcsos_bcache_t;

void mcsos_blk_registry_reset(void);
mcsos_blk_status_t mcsos_blk_register(mcsos_blk_device_t *dev);
mcsos_blk_device_t *mcsos_blk_get(uint32_t index);
uint32_t mcsos_blk_count(void);
mcsos_blk_status_t mcsos_blk_read(mcsos_blk_device_t *dev, uint64_t lba, uint32_t count, void *buffer);
mcsos_blk_status_t mcsos_blk_write(mcsos_blk_device_t *dev, uint64_t lba, uint32_t count, const void *buffer);
mcsos_blk_status_t mcsos_blk_flush(mcsos_blk_device_t *dev);

mcsos_blk_status_t mcsos_ramblk_init(mcsos_blk_device_t *dev,
                                     mcsos_ramblk_t *ram,
                                     const char *name,
                                     uint8_t *storage,
                                     uint64_t storage_size,
                                     uint32_t block_size);

mcsos_blk_status_t mcsos_bcache_init(mcsos_bcache_t *cache,
                                     mcsos_bcache_entry_t *entries,
                                     uint32_t entry_count,
                                     uint8_t *data_pool,
                                     uint32_t block_size);
mcsos_blk_status_t mcsos_bcache_read(mcsos_bcache_t *cache,
                                     mcsos_blk_device_t *dev,
                                     uint64_t lba,
                                     void *buffer);
mcsos_blk_status_t mcsos_bcache_write(mcsos_bcache_t *cache,
                                      mcsos_blk_device_t *dev,
                                      uint64_t lba,
                                      const void *buffer);
mcsos_blk_status_t mcsos_bcache_flush_all(mcsos_bcache_t *cache);

#endif

