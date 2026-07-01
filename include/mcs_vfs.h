#ifndef MCS_VFS_H
#define MCS_VFS_H

#include <stddef.h>
#include <stdint.h>

#define MCS_MAX_NAME 32u
#define MCS_MAX_PATH 128u
#define MCS_MAX_NODES 64u
#define MCS_MAX_OPEN_FILES 16u
#define MCS_RAMFS_DATA_BYTES 8192u

#define MCS_O_RDONLY 0x0001u
#define MCS_O_WRONLY 0x0002u
#define MCS_O_RDWR   0x0004u
#define MCS_O_CREAT  0x0100u
#define MCS_O_TRUNC  0x0200u
#define MCS_O_APPEND 0x0400u

#define MCS_SEEK_SET 0
#define MCS_SEEK_CUR 1
#define MCS_SEEK_END 2

typedef long mcs_ssize_t;

typedef enum mcs_vnode_type {
    MCS_VNODE_DIR = 1,
    MCS_VNODE_FILE = 2
} mcs_vnode_type_t;

typedef enum mcs_vfs_status {
    MCS_OK = 0,
    MCS_ENOENT = -2,
    MCS_EBADF = -9,
    MCS_EACCES = -13,
    MCS_EEXIST = -17,
    MCS_ENOTDIR = -20,
    MCS_EISDIR = -21,
    MCS_EINVAL = -22,
    MCS_ENFILE = -23,
    MCS_ENOSPC = -28,
    MCS_ENAMETOOLONG = -36
} mcs_vfs_status_t;

typedef struct mcs_vnode {
    uint32_t used;
    uint32_t id;
    uint32_t parent;
    mcs_vnode_type_t type;
    char name[MCS_MAX_NAME];
    size_t size;
    size_t data_offset;
    size_t data_capacity;
} mcs_vnode_t;

typedef struct mcs_ramfs {
    mcs_vnode_t nodes[MCS_MAX_NODES];
    size_t node_count;
    uint8_t data[MCS_RAMFS_DATA_BYTES];
    size_t data_used;
} mcs_ramfs_t;

typedef struct mcs_file {
    uint32_t used;
    uint32_t flags;
    size_t offset;
    mcs_vnode_t *node;
    mcs_ramfs_t *fs;
} mcs_file_t;

typedef struct mcs_fd_table {
    mcs_file_t files[MCS_MAX_OPEN_FILES];
} mcs_fd_table_t;

typedef struct mcs_process {
    uint32_t pid;
    mcs_fd_table_t fd_table;
} mcs_process_t;

void mcs_ramfs_init(mcs_ramfs_t *fs);
int mcs_ramfs_seed_file(mcs_ramfs_t *fs, const char *path, const uint8_t *data, size_t len);
int mcs_ramfs_lookup(mcs_ramfs_t *fs, const char *path, mcs_vnode_t **out_node);
int mcs_ramfs_create_file(mcs_ramfs_t *fs, const char *path, mcs_vnode_t **out_node);

void mcs_fd_table_init(mcs_fd_table_t *table);
int mcs_vfs_open(mcs_fd_table_t *table, mcs_ramfs_t *fs, const char *path, uint32_t flags);
mcs_ssize_t mcs_vfs_read(mcs_fd_table_t *table, int fd, void *buf, size_t len);
mcs_ssize_t mcs_vfs_write(mcs_fd_table_t *table, int fd, const void *buf, size_t len);
int mcs_vfs_lseek(mcs_fd_table_t *table, int fd, long offset, int whence);
int mcs_vfs_close(mcs_fd_table_t *table, int fd);
int mcs_vfs_dup(mcs_fd_table_t *table, int fd);

int mcs_sys_open(mcs_process_t *proc, mcs_ramfs_t *fs, const char *user_path, uint32_t flags);
mcs_ssize_t mcs_sys_read(mcs_process_t *proc, int fd, void *user_buf, size_t len);
mcs_ssize_t mcs_sys_write(mcs_process_t *proc, int fd, const void *user_buf, size_t len);
int mcs_sys_close(mcs_process_t *proc, int fd);
int mcs_sys_lseek(mcs_process_t *proc, int fd, long offset, int whence);

#endif


