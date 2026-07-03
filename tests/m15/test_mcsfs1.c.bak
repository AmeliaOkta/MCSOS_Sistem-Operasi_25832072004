#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "../../fs/mcsfs1/mcsfs1.h"

#define RAMBLK_BLOCKS 128u
static uint8_t disk[RAMBLK_BLOCKS][MCSFS1_BLOCK_SIZE];
static unsigned flush_count;

static int ram_read(void *ctx, uint32_t lba, void *buf512) {
    (void)ctx;
    if (lba >= RAMBLK_BLOCKS) return -1;
    memcpy(buf512, disk[lba], MCSFS1_BLOCK_SIZE);
    return 0;
}

static int ram_write(void *ctx, uint32_t lba, const void *buf512) {
    (void)ctx;
    if (lba >= RAMBLK_BLOCKS) return -1;
    memcpy(disk[lba], buf512, MCSFS1_BLOCK_SIZE);
    return 0;
}

static int ram_flush(void *ctx) {
    (void)ctx;
    flush_count++;
    return 0;
}

static int expect_int(const char *label, int got, int want) {
    if (got != want) {
        printf("FAIL %s got=%d want=%d\n", label, got, want);
        return 1;
    }
    return 0;
}

int main(void) {
    struct mcsfs1_blkdev dev = {0};
    struct mcsfs1_mount mnt = {0};
    uint8_t out[4096];
    uint32_t out_len = 0;
    int fails = 0;

    dev.block_count = RAMBLK_BLOCKS;
    dev.read = ram_read;
    dev.write = ram_write;
    dev.flush = ram_flush;

    fails += expect_int("format", mcsfs1_format(&dev), MCSFS1_ERR_OK);
    fails += expect_int("mount", mcsfs1_mount(&mnt, &dev), MCSFS1_ERR_OK);
    fails += expect_int("fsck-empty", mcsfs1_fsck(&dev), MCSFS1_ERR_OK);
    fails += expect_int("create-alpha", mcsfs1_create(&mnt, "alpha.txt"), MCSFS1_ERR_OK);
    fails += expect_int("create-duplicate", mcsfs1_create(&mnt, "alpha.txt"), MCSFS1_ERR_EXIST);

    const char msg[] = "MCSOS M15 persistent file payload";
    fails += expect_int("write-alpha", mcsfs1_write(&mnt, "alpha.txt", (const uint8_t *)msg, (uint32_t)strlen(msg)), MCSFS1_ERR_OK);
    memset(out, 0, sizeof(out));
    fails += expect_int("read-alpha", mcsfs1_read(&mnt, "alpha.txt", out, sizeof(out), &out_len), MCSFS1_ERR_OK);
    if (out_len != strlen(msg) || memcmp(out, msg, strlen(msg)) != 0) {
        printf("FAIL read-data len=%u\n", out_len);
        fails++;
    }

    uint8_t big[1400];
    for (unsigned i = 0; i < sizeof(big); i++) big[i] = (uint8_t)(i & 0xffu);
    fails += expect_int("write-big", mcsfs1_write(&mnt, "alpha.txt", big, sizeof(big)), MCSFS1_ERR_OK);
    memset(out, 0, sizeof(out));
    fails += expect_int("read-big", mcsfs1_read(&mnt, "alpha.txt", out, sizeof(out), &out_len), MCSFS1_ERR_OK);
    if (out_len != sizeof(big) || memcmp(out, big, sizeof(big)) != 0) {
        printf("FAIL read-big-data len=%u\n", out_len);
        fails++;
    }

    fails += expect_int("read-small-cap", mcsfs1_read(&mnt, "alpha.txt", out, 8, &out_len), MCSFS1_ERR_RANGE);
    fails += expect_int("missing", mcsfs1_read(&mnt, "missing", out, sizeof(out), &out_len), MCSFS1_ERR_NOENT);
    fails += expect_int("fsck-populated", mcsfs1_fsck(&dev), MCSFS1_ERR_OK);
    fails += expect_int("unlink", mcsfs1_unlink(&mnt, "alpha.txt"), MCSFS1_ERR_OK);
    fails += expect_int("read-after-unlink", mcsfs1_read(&mnt, "alpha.txt", out, sizeof(out), &out_len), MCSFS1_ERR_NOENT);
    fails += expect_int("fsck-after-unlink", mcsfs1_fsck(&dev), MCSFS1_ERR_OK);

    disk[0][0] ^= 0x55u;
    fails += expect_int("corrupt-super", mcsfs1_fsck(&dev), MCSFS1_ERR_CORRUPT);

    if (flush_count == 0) {
        printf("FAIL flush-count zero\n");
        fails++;
    }

    if (fails != 0) {
        printf("M15 host test failed: %d failures\n", fails);
        return 1;
    }
    printf("M15 host test passed: flush_count=%u\n", flush_count);
    return 0;
}
