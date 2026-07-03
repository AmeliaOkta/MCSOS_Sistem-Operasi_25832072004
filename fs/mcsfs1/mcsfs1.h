#ifndef MCSFS1_H
#define MCSFS1_H

#include <stdint.h>
#include <stddef.h>

#define MCSFS1_BLOCK_SIZE 512u
#define MCSFS1_MAGIC 0x31465343u
#define MCSFS1_VERSION 1u
#define MCSFS1_MAX_INODES 32u
#define MCSFS1_DIRECT_BLOCKS 8u
#define MCSFS1_MAX_NAME 27u
#define MCSFS1_ROOT_INO 1u
#define MCSFS1_MODE_FREE 0u
#define MCSFS1_MODE_FILE 1u
#define MCSFS1_MODE_DIR 2u
#define MCSFS1_ERR_OK 0
#define MCSFS1_ERR_INVAL -1
#define MCSFS1_ERR_IO -2
#define MCSFS1_ERR_NOSPC -3
#define MCSFS1_ERR_EXIST -4
#define MCSFS1_ERR_NOENT -5
#define MCSFS1_ERR_NAMETOOLONG -6
#define MCSFS1_ERR_CORRUPT -7
#define MCSFS1_ERR_ISDIR -8
#define MCSFS1_ERR_RANGE -9

struct mcsfs1_blkdev {
    void *ctx;
    uint32_t block_count;
    int (*read)(void *ctx, uint32_t lba, void *buf512);
    int (*write)(void *ctx, uint32_t lba, const void *buf512);
    int (*flush)(void *ctx);
};

struct mcsfs1_mount {
    struct mcsfs1_blkdev *dev;
    uint32_t block_count;
    uint32_t data_start;
};

int mcsfs1_format(struct mcsfs1_blkdev *dev);
int mcsfs1_mount(struct mcsfs1_mount *mnt, struct mcsfs1_blkdev *dev);
int mcsfs1_fsck(struct mcsfs1_blkdev *dev);
int mcsfs1_create(struct mcsfs1_mount *mnt, const char *name);
int mcsfs1_write(struct mcsfs1_mount *mnt, const char *name, const uint8_t *buf, uint32_t len);
int mcsfs1_read(struct mcsfs1_mount *mnt, const char *name, uint8_t *buf, uint32_t cap, uint32_t *out_len);
int mcsfs1_unlink(struct mcsfs1_mount *mnt, const char *name);

#endif

