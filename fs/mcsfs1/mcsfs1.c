#include "mcsfs1.h"

#define MCSFS1_SB_LBA 0u
#define MCSFS1_INODE_BMAP_LBA 1u
#define MCSFS1_BLOCK_BMAP_LBA 2u
#define MCSFS1_INODE_TABLE_LBA 3u
#define MCSFS1_INODE_TABLE_BLOCKS 4u
#define MCSFS1_ROOT_DIR_LBA 7u
#define MCSFS1_DATA_START_LBA 8u
#define MCSFS1_MIN_BLOCKS 16u
#define MCSFS1_DIRENT_COUNT 16u

struct mcsfs1_super_disk {
    uint32_t magic;
    uint32_t version;
    uint32_t block_size;
    uint32_t block_count;
    uint32_t inode_count;
    uint32_t inode_bmap_lba;
    uint32_t block_bmap_lba;
    uint32_t inode_table_lba;
    uint32_t inode_table_blocks;
    uint32_t root_ino;
    uint32_t root_dir_lba;
    uint32_t data_start_lba;
    uint32_t clean;
    uint32_t reserved[115];
};

struct mcsfs1_inode_disk {
    uint16_t mode;
    uint16_t links;
    uint32_t size;
    uint32_t direct[MCSFS1_DIRECT_BLOCKS];
    uint32_t reserved[5];
};

struct mcsfs1_dirent_disk {
    uint32_t ino;
    uint8_t type;
    char name[MCSFS1_MAX_NAME];
};

static void *mcsfs_memset(void *dst, int c, uint32_t n) {
    uint8_t *d = (uint8_t *)dst;
    for (uint32_t i = 0; i < n; i++) {
        d[i] = (uint8_t)c;
    }
    return dst;
}

static void *mcsfs_memcpy(void *dst, const void *src, uint32_t n) {
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    for (uint32_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dst;
}

static int mcsfs_memcmp(const void *a, const void *b, uint32_t n) {
    const uint8_t *x = (const uint8_t *)a;
    const uint8_t *y = (const uint8_t *)b;
    for (uint32_t i = 0; i < n; i++) {
        if (x[i] != y[i]) {
            return (int)x[i] - (int)y[i];
        }
    }
    return 0;
}

static uint32_t mcsfs_strlen_bound(const char *s, uint32_t max_plus_one) {
    uint32_t n = 0;
    if (s == 0) {
        return max_plus_one;
    }
    while (n < max_plus_one && s[n] != '\0') {
        n++;
    }
    return n;
}

static int valid_name(const char *name, uint32_t *len_out) {
    uint32_t n = mcsfs_strlen_bound(name, MCSFS1_MAX_NAME + 1u);
    if (n == 0u) {
        return MCSFS1_ERR_INVAL;
    }
    if (n > MCSFS1_MAX_NAME) {
        return MCSFS1_ERR_NAMETOOLONG;
    }
    for (uint32_t i = 0; i < n; i++) {
        if (name[i] == '/') {
            return MCSFS1_ERR_INVAL;
        }
    }
    *len_out = n;
    return MCSFS1_ERR_OK;
}

static int dev_read(struct mcsfs1_blkdev *dev, uint32_t lba, void *buf) {
    if (dev == 0 || dev->read == 0 || buf == 0 || lba >= dev->block_count) {
        return MCSFS1_ERR_INVAL;
    }
    return dev->read(dev->ctx, lba, buf) == 0 ? MCSFS1_ERR_OK : MCSFS1_ERR_IO;
}

static int dev_write(struct mcsfs1_blkdev *dev, uint32_t lba, const void *buf) {
    if (dev == 0 || dev->write == 0 || buf == 0 || lba >= dev->block_count) {
        return MCSFS1_ERR_INVAL;
    }
    return dev->write(dev->ctx, lba, buf) == 0 ? MCSFS1_ERR_OK : MCSFS1_ERR_IO;
}

static int dev_flush(struct mcsfs1_blkdev *dev) {
    if (dev == 0 || dev->flush == 0) {
        return MCSFS1_ERR_INVAL;
    }
    return dev->flush(dev->ctx) == 0 ? MCSFS1_ERR_OK : MCSFS1_ERR_IO;
}

static void bit_set(uint8_t *b, uint32_t bit) {
    b[bit / 8u] = (uint8_t)(b[bit / 8u] | (uint8_t)(1u << (bit % 8u)));
}

static void bit_clear(uint8_t *b, uint32_t bit) {
    b[bit / 8u] = (uint8_t)(b[bit / 8u] & (uint8_t)~(uint8_t)(1u << (bit % 8u)));
}

static int bit_test(const uint8_t *b, uint32_t bit) {
    return (b[bit / 8u] & (uint8_t)(1u << (bit % 8u))) != 0u;
}

static int load_super(struct mcsfs1_blkdev *dev, struct mcsfs1_super_disk *sb) {
    int rc = dev_read(dev, MCSFS1_SB_LBA, sb);
    if (rc != 0) {
        return rc;
    }
    if (sb->magic != MCSFS1_MAGIC || sb->version != MCSFS1_VERSION || sb->block_size != MCSFS1_BLOCK_SIZE) {
        return MCSFS1_ERR_CORRUPT;
    }
    if (sb->block_count != dev->block_count || sb->inode_count != MCSFS1_MAX_INODES) {
        return MCSFS1_ERR_CORRUPT;
    }
    if (sb->inode_bmap_lba != MCSFS1_INODE_BMAP_LBA || sb->block_bmap_lba != MCSFS1_BLOCK_BMAP_LBA || sb->inode_table_lba != MCSFS1_INODE_TABLE_LBA) {
        return MCSFS1_ERR_CORRUPT;
    }
    if (sb->root_ino != MCSFS1_ROOT_INO || sb->root_dir_lba != MCSFS1_ROOT_DIR_LBA || sb->data_start_lba != MCSFS1_DATA_START_LBA) {
        return MCSFS1_ERR_CORRUPT;
    }
    if (sb->data_start_lba >= sb->block_count) {
        return MCSFS1_ERR_CORRUPT;
    }
    return MCSFS1_ERR_OK;
}

static int read_inode(struct mcsfs1_blkdev *dev, uint32_t ino, struct mcsfs1_inode_disk *inode) {
    if (ino == 0u || ino > MCSFS1_MAX_INODES || inode == 0) {
        return MCSFS1_ERR_INVAL;
    }
    uint8_t block[MCSFS1_BLOCK_SIZE];
    uint32_t index = ino - 1u;
    uint32_t per_block = MCSFS1_BLOCK_SIZE / (uint32_t)sizeof(struct mcsfs1_inode_disk);
    uint32_t lba = MCSFS1_INODE_TABLE_LBA + (index / per_block);
    uint32_t off = (index % per_block) * (uint32_t)sizeof(struct mcsfs1_inode_disk);
    if (lba >= MCSFS1_DATA_START_LBA) {
        return MCSFS1_ERR_CORRUPT;
    }
    int rc = dev_read(dev, lba, block);
    if (rc != 0) {
        return rc;
    }
    mcsfs_memcpy(inode, block + off, (uint32_t)sizeof(*inode));
    return MCSFS1_ERR_OK;
}

static int write_inode(struct mcsfs1_blkdev *dev, uint32_t ino, const struct mcsfs1_inode_disk *inode) {
    if (ino == 0u || ino > MCSFS1_MAX_INODES || inode == 0) {
        return MCSFS1_ERR_INVAL;
    }
    uint8_t block[MCSFS1_BLOCK_SIZE];
    uint32_t index = ino - 1u;
    uint32_t per_block = MCSFS1_BLOCK_SIZE / (uint32_t)sizeof(struct mcsfs1_inode_disk);
    uint32_t lba = MCSFS1_INODE_TABLE_LBA + (index / per_block);
    uint32_t off = (index % per_block) * (uint32_t)sizeof(struct mcsfs1_inode_disk);
    if (lba >= MCSFS1_DATA_START_LBA) {
        return MCSFS1_ERR_CORRUPT;
    }
    int rc = dev_read(dev, lba, block);
    if (rc != 0) {
        return rc;
    }
    mcsfs_memcpy(block + off, inode, (uint32_t)sizeof(*inode));
    return dev_write(dev, lba, block);
}

static int load_bmaps(struct mcsfs1_blkdev *dev, uint8_t *ib, uint8_t *bb) {
    int rc = dev_read(dev, MCSFS1_INODE_BMAP_LBA, ib);
    if (rc != 0) {
        return rc;
    }
    return dev_read(dev, MCSFS1_BLOCK_BMAP_LBA, bb);
}

static int store_bmaps(struct mcsfs1_blkdev *dev, const uint8_t *ib, const uint8_t *bb) {
    int rc = dev_write(dev, MCSFS1_INODE_BMAP_LBA, ib);
    if (rc != 0) {
        return rc;
    }
    return dev_write(dev, MCSFS1_BLOCK_BMAP_LBA, bb);
}

static int find_dirent(struct mcsfs1_blkdev *dev, const char *name, uint32_t *slot_out, uint32_t *ino_out) {
    uint8_t block[MCSFS1_BLOCK_SIZE];
    uint32_t name_len = 0;
    int rc = valid_name(name, &name_len);
    if (rc != 0) {
        return rc;
    }
    rc = dev_read(dev, MCSFS1_ROOT_DIR_LBA, block);
    if (rc != 0) {
        return rc;
    }
    struct mcsfs1_dirent_disk *de = (struct mcsfs1_dirent_disk *)block;
    for (uint32_t i = 0; i < MCSFS1_DIRENT_COUNT; i++) {
        if (de[i].ino != 0u && mcsfs_strlen_bound(de[i].name, MCSFS1_MAX_NAME + 1u) == name_len && mcsfs_memcmp(de[i].name, name, name_len) == 0) {
            if (slot_out != 0) {
                *slot_out = i;
            }
            if (ino_out != 0) {
                *ino_out = de[i].ino;
            }
            return MCSFS1_ERR_OK;
        }
    }
    return MCSFS1_ERR_NOENT;
}

static int alloc_inode_block(struct mcsfs1_blkdev *dev, uint32_t *ino_out, uint32_t *data_lba_out) {
    uint8_t ib[MCSFS1_BLOCK_SIZE];
    uint8_t bb[MCSFS1_BLOCK_SIZE];
    int rc = load_bmaps(dev, ib, bb);
    if (rc != 0) {
        return rc;
    }
    uint32_t ino = 0;
    for (uint32_t i = 2u; i <= MCSFS1_MAX_INODES; i++) {
        if (!bit_test(ib, i)) {
            ino = i;
            break;
        }
    }
    if (ino == 0u) {
        return MCSFS1_ERR_NOSPC;
    }
    uint32_t lba = 0;
    for (uint32_t b = MCSFS1_DATA_START_LBA; b < dev->block_count; b++) {
        if (!bit_test(bb, b)) {
            lba = b;
            break;
        }
    }
    if (lba == 0u) {
        return MCSFS1_ERR_NOSPC;
    }
    bit_set(ib, ino);
    bit_set(bb, lba);
    rc = store_bmaps(dev, ib, bb);
    if (rc != 0) {
        return rc;
    }
    *ino_out = ino;
    *data_lba_out = lba;
    return MCSFS1_ERR_OK;
}

static int alloc_data_block(struct mcsfs1_blkdev *dev, uint32_t *data_lba_out) {
    uint8_t ib[MCSFS1_BLOCK_SIZE];
    uint8_t bb[MCSFS1_BLOCK_SIZE];
    int rc = load_bmaps(dev, ib, bb);
    if (rc != 0) {
        return rc;
    }
    for (uint32_t b = MCSFS1_DATA_START_LBA; b < dev->block_count; b++) {
        if (!bit_test(bb, b)) {
            bit_set(bb, b);
            rc = store_bmaps(dev, ib, bb);
            if (rc != 0) {
                return rc;
            }
            *data_lba_out = b;
            return MCSFS1_ERR_OK;
        }
    }
    return MCSFS1_ERR_NOSPC;
}

static int free_inode_and_blocks(struct mcsfs1_blkdev *dev, uint32_t ino, const struct mcsfs1_inode_disk *inode) {
    uint8_t ib[MCSFS1_BLOCK_SIZE];
    uint8_t bb[MCSFS1_BLOCK_SIZE];
    int rc = load_bmaps(dev, ib, bb);
    if (rc != 0) {
        return rc;
    }
    bit_clear(ib, ino);
    for (uint32_t i = 0; i < MCSFS1_DIRECT_BLOCKS; i++) {
        if (inode->direct[i] != 0u && inode->direct[i] < dev->block_count) {
            bit_clear(bb, inode->direct[i]);
        }
    }
    return store_bmaps(dev, ib, bb);
}

int mcsfs1_format(struct mcsfs1_blkdev *dev) {
    if (dev == 0 || dev->block_count < MCSFS1_MIN_BLOCKS || dev->block_count > (MCSFS1_BLOCK_SIZE * 8u)) {
        return MCSFS1_ERR_INVAL;
    }
    uint8_t zero[MCSFS1_BLOCK_SIZE];
    mcsfs_memset(zero, 0, MCSFS1_BLOCK_SIZE);
    for (uint32_t lba = 0; lba < dev->block_count; lba++) {
        int rc0 = dev_write(dev, lba, zero);
        if (rc0 != 0) {
            return rc0;
        }
    }

    struct mcsfs1_super_disk sb;
    mcsfs_memset(&sb, 0, (uint32_t)sizeof(sb));
    sb.magic = MCSFS1_MAGIC;
    sb.version = MCSFS1_VERSION;
    sb.block_size = MCSFS1_BLOCK_SIZE;
    sb.block_count = dev->block_count;
    sb.inode_count = MCSFS1_MAX_INODES;
    sb.inode_bmap_lba = MCSFS1_INODE_BMAP_LBA;
    sb.block_bmap_lba = MCSFS1_BLOCK_BMAP_LBA;
    sb.inode_table_lba = MCSFS1_INODE_TABLE_LBA;
    sb.inode_table_blocks = MCSFS1_INODE_TABLE_BLOCKS;
    sb.root_ino = MCSFS1_ROOT_INO;
    sb.root_dir_lba = MCSFS1_ROOT_DIR_LBA;
    sb.data_start_lba = MCSFS1_DATA_START_LBA;
    sb.clean = 1u;
    int rc = dev_write(dev, MCSFS1_SB_LBA, &sb);
    if (rc != 0) {
        return rc;
    }

    uint8_t ib[MCSFS1_BLOCK_SIZE];
    uint8_t bb[MCSFS1_BLOCK_SIZE];
    mcsfs_memset(ib, 0, MCSFS1_BLOCK_SIZE);
    mcsfs_memset(bb, 0, MCSFS1_BLOCK_SIZE);
    bit_set(ib, 0u);
    bit_set(ib, MCSFS1_ROOT_INO);
    for (uint32_t b = 0; b < MCSFS1_DATA_START_LBA; b++) {
        bit_set(bb, b);
    }
    bit_set(bb, MCSFS1_ROOT_DIR_LBA);
    rc = store_bmaps(dev, ib, bb);
    if (rc != 0) {
        return rc;
    }

    struct mcsfs1_inode_disk root;
    mcsfs_memset(&root, 0, (uint32_t)sizeof(root));
    root.mode = MCSFS1_MODE_DIR;
    root.links = 1u;
    root.size = MCSFS1_BLOCK_SIZE;
    root.direct[0] = MCSFS1_ROOT_DIR_LBA;
    rc = write_inode(dev, MCSFS1_ROOT_INO, &root);
    if (rc != 0) {
        return rc;
    }
    return dev_flush(dev);
}

int mcsfs1_mount(struct mcsfs1_mount *mnt, struct mcsfs1_blkdev *dev) {
    if (mnt == 0 || dev == 0) {
        return MCSFS1_ERR_INVAL;
    }
    struct mcsfs1_super_disk sb;
    int rc = load_super(dev, &sb);
    if (rc != 0) {
        return rc;
    }
    struct mcsfs1_inode_disk root;
    rc = read_inode(dev, MCSFS1_ROOT_INO, &root);
    if (rc != 0) {
        return rc;
    }
    if (root.mode != MCSFS1_MODE_DIR || root.direct[0] != MCSFS1_ROOT_DIR_LBA) {
        return MCSFS1_ERR_CORRUPT;
    }
    mnt->dev = dev;
    mnt->block_count = sb.block_count;
    mnt->data_start = sb.data_start_lba;
    return MCSFS1_ERR_OK;
}

int mcsfs1_create(struct mcsfs1_mount *mnt, const char *name) {
    if (mnt == 0 || mnt->dev == 0) {
        return MCSFS1_ERR_INVAL;
    }
    uint32_t name_len = 0;
    int rc = valid_name(name, &name_len);
    if (rc != 0) {
        return rc;
    }
    if (find_dirent(mnt->dev, name, 0, 0) == 0) {
        return MCSFS1_ERR_EXIST;
    }
    uint8_t dir_block[MCSFS1_BLOCK_SIZE];
    rc = dev_read(mnt->dev, MCSFS1_ROOT_DIR_LBA, dir_block);
    if (rc != 0) {
        return rc;
    }
    struct mcsfs1_dirent_disk *de = (struct mcsfs1_dirent_disk *)dir_block;
    uint32_t free_slot = MCSFS1_DIRENT_COUNT;
    for (uint32_t i = 0; i < MCSFS1_DIRENT_COUNT; i++) {
        if (de[i].ino == 0u) {
            free_slot = i;
            break;
        }
    }
    if (free_slot == MCSFS1_DIRENT_COUNT) {
        return MCSFS1_ERR_NOSPC;
    }
    uint32_t ino = 0;
    uint32_t first_data = 0;
    rc = alloc_inode_block(mnt->dev, &ino, &first_data);
    if (rc != 0) {
        return rc;
    }
    struct mcsfs1_inode_disk inode;
    mcsfs_memset(&inode, 0, (uint32_t)sizeof(inode));
    inode.mode = MCSFS1_MODE_FILE;
    inode.links = 1u;
    inode.size = 0u;
    inode.direct[0] = first_data;
    rc = write_inode(mnt->dev, ino, &inode);
    if (rc != 0) {
        return rc;
    }
    de[free_slot].ino = ino;
    de[free_slot].type = MCSFS1_MODE_FILE;
    mcsfs_memset(de[free_slot].name, 0, MCSFS1_MAX_NAME);
    mcsfs_memcpy(de[free_slot].name, name, name_len);
    rc = dev_write(mnt->dev, MCSFS1_ROOT_DIR_LBA, dir_block);
    if (rc != 0) {
        return rc;
    }
    return dev_flush(mnt->dev);
}

int mcsfs1_write(struct mcsfs1_mount *mnt, const char *name, const uint8_t *buf, uint32_t len) {
    if (mnt == 0 || mnt->dev == 0 || (buf == 0 && len != 0u)) {
        return MCSFS1_ERR_INVAL;
    }
    if (len > MCSFS1_DIRECT_BLOCKS * MCSFS1_BLOCK_SIZE) {
        return MCSFS1_ERR_RANGE;
    }
    uint32_t ino = 0;
    int rc = find_dirent(mnt->dev, name, 0, &ino);
    if (rc != 0) {
        return rc;
    }
    struct mcsfs1_inode_disk inode;
    rc = read_inode(mnt->dev, ino, &inode);
    if (rc != 0) {
        return rc;
    }
    if (inode.mode != MCSFS1_MODE_FILE) {
        return MCSFS1_ERR_ISDIR;
    }
    uint32_t blocks_needed = (len + MCSFS1_BLOCK_SIZE - 1u) / MCSFS1_BLOCK_SIZE;
    if (blocks_needed == 0u) {
        blocks_needed = 1u;
    }
    for (uint32_t i = 0; i < blocks_needed; i++) {
        if (inode.direct[i] == 0u) {
            rc = alloc_data_block(mnt->dev, &inode.direct[i]);
            if (rc != 0) {
                return rc;
            }
        }
    }
    uint8_t block[MCSFS1_BLOCK_SIZE];
    uint32_t written = 0;
    for (uint32_t i = 0; i < blocks_needed; i++) {
        mcsfs_memset(block, 0, MCSFS1_BLOCK_SIZE);
        uint32_t remain = len - written;
        uint32_t chunk = remain > MCSFS1_BLOCK_SIZE ? MCSFS1_BLOCK_SIZE : remain;
        if (chunk != 0u) {
            mcsfs_memcpy(block, buf + written, chunk);
        }
        rc = dev_write(mnt->dev, inode.direct[i], block);
        if (rc != 0) {
            return rc;
        }
        written += chunk;
    }
    inode.size = len;
    rc = write_inode(mnt->dev, ino, &inode);
    if (rc != 0) {
        return rc;
    }
    return dev_flush(mnt->dev);
}

int mcsfs1_read(struct mcsfs1_mount *mnt, const char *name, uint8_t *buf, uint32_t cap, uint32_t *out_len) {
    if (mnt == 0 || mnt->dev == 0 || buf == 0 || out_len == 0) {
        return MCSFS1_ERR_INVAL;
    }
    uint32_t ino = 0;
    int rc = find_dirent(mnt->dev, name, 0, &ino);
    if (rc != 0) {
        return rc;
    }
    struct mcsfs1_inode_disk inode;
    rc = read_inode(mnt->dev, ino, &inode);
    if (rc != 0) {
        return rc;
    }
    if (inode.mode != MCSFS1_MODE_FILE) {
        return MCSFS1_ERR_ISDIR;
    }
    if (cap < inode.size) {
        return MCSFS1_ERR_RANGE;
    }
    uint32_t blocks_needed = (inode.size + MCSFS1_BLOCK_SIZE - 1u) / MCSFS1_BLOCK_SIZE;
    uint32_t copied = 0;
    uint8_t block[MCSFS1_BLOCK_SIZE];
    for (uint32_t i = 0; i < blocks_needed; i++) {
        if (inode.direct[i] == 0u || inode.direct[i] >= mnt->block_count) {
            return MCSFS1_ERR_CORRUPT;
        }
        rc = dev_read(mnt->dev, inode.direct[i], block);
        if (rc != 0) {
            return rc;
        }
        uint32_t remain = inode.size - copied;
        uint32_t chunk = remain > MCSFS1_BLOCK_SIZE ? MCSFS1_BLOCK_SIZE : remain;
        mcsfs_memcpy(buf + copied, block, chunk);
        copied += chunk;
    }
    *out_len = inode.size;
    return MCSFS1_ERR_OK;
}

int mcsfs1_unlink(struct mcsfs1_mount *mnt, const char *name) {
    if (mnt == 0 || mnt->dev == 0) {
        return MCSFS1_ERR_INVAL;
    }
    uint32_t slot = 0;
    uint32_t ino = 0;
    int rc = find_dirent(mnt->dev, name, &slot, &ino);
    if (rc != 0) {
        return rc;
    }
    struct mcsfs1_inode_disk inode;
    rc = read_inode(mnt->dev, ino, &inode);
    if (rc != 0) {
        return rc;
    }
    if (inode.mode != MCSFS1_MODE_FILE) {
        return MCSFS1_ERR_ISDIR;
    }
    uint8_t dir_block[MCSFS1_BLOCK_SIZE];
    rc = dev_read(mnt->dev, MCSFS1_ROOT_DIR_LBA, dir_block);
    if (rc != 0) {
        return rc;
    }
    struct mcsfs1_dirent_disk *de = (struct mcsfs1_dirent_disk *)dir_block;
    mcsfs_memset(&de[slot], 0, (uint32_t)sizeof(de[slot]));
    rc = dev_write(mnt->dev, MCSFS1_ROOT_DIR_LBA, dir_block);
    if (rc != 0) {
        return rc;
    }
    rc = free_inode_and_blocks(mnt->dev, ino, &inode);
    if (rc != 0) {
        return rc;
    }
    struct mcsfs1_inode_disk zero_inode;
    mcsfs_memset(&zero_inode, 0, (uint32_t)sizeof(zero_inode));
    rc = write_inode(mnt->dev, ino, &zero_inode);
    if (rc != 0) {
        return rc;
    }
    return dev_flush(mnt->dev);
}

int mcsfs1_fsck(struct mcsfs1_blkdev *dev) {
    if (dev == 0) {
        return MCSFS1_ERR_INVAL;
    }
    struct mcsfs1_super_disk sb;
    int rc = load_super(dev, &sb);
    if (rc != 0) {
        return rc;
    }
    uint8_t ib[MCSFS1_BLOCK_SIZE];
    uint8_t bb[MCSFS1_BLOCK_SIZE];
    rc = load_bmaps(dev, ib, bb);
    if (rc != 0) {
        return rc;
    }
    if (!bit_test(ib, MCSFS1_ROOT_INO) || !bit_test(bb, MCSFS1_ROOT_DIR_LBA)) {
        return MCSFS1_ERR_CORRUPT;
    }
    for (uint32_t b = 0; b < MCSFS1_DATA_START_LBA; b++) {
        if (!bit_test(bb, b)) {
            return MCSFS1_ERR_CORRUPT;
        }
    }
    struct mcsfs1_inode_disk root;
    rc = read_inode(dev, MCSFS1_ROOT_INO, &root);
    if (rc != 0) {
        return rc;
    }
    if (root.mode != MCSFS1_MODE_DIR || root.direct[0] != MCSFS1_ROOT_DIR_LBA) {
        return MCSFS1_ERR_CORRUPT;
    }
    uint8_t dir_block[MCSFS1_BLOCK_SIZE];
    rc = dev_read(dev, MCSFS1_ROOT_DIR_LBA, dir_block);
    if (rc != 0) {
        return rc;
    }
    struct mcsfs1_dirent_disk *de = (struct mcsfs1_dirent_disk *)dir_block;
    for (uint32_t i = 0; i < MCSFS1_DIRENT_COUNT; i++) {
        if (de[i].ino == 0u) {
            continue;
        }
        if (de[i].ino > MCSFS1_MAX_INODES || de[i].type != MCSFS1_MODE_FILE || !bit_test(ib, de[i].ino)) {
            return MCSFS1_ERR_CORRUPT;
        }
        struct mcsfs1_inode_disk inode;
        rc = read_inode(dev, de[i].ino, &inode);
        if (rc != 0) {
            return rc;
        }
        if (inode.mode != MCSFS1_MODE_FILE || inode.size > MCSFS1_DIRECT_BLOCKS * MCSFS1_BLOCK_SIZE) {
            return MCSFS1_ERR_CORRUPT;
        }
        uint32_t needed = (inode.size + MCSFS1_BLOCK_SIZE - 1u) / MCSFS1_BLOCK_SIZE;
        if (needed == 0u) {
            needed = 1u;
        }
        for (uint32_t j = 0; j < needed; j++) {
            uint32_t lba = inode.direct[j];
            if (lba < MCSFS1_DATA_START_LBA || lba >= dev->block_count || !bit_test(bb, lba)) {
                return MCSFS1_ERR_CORRUPT;
            }
        }
    }
    return MCSFS1_ERR_OK;
}
