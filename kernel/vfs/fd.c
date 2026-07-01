#include "mcs_vfs.h"

static size_t mcs_min_size(size_t a, size_t b) {
    return a < b ? a : b;
}

static void mcs_copy_to_user(void *dst, const uint8_t *src, size_t n) {
    size_t i;
    uint8_t *d = (uint8_t *)dst;
    for (i = 0; i < n; i++) {
        d[i] = src[i];
    }
}

static void mcs_copy_from_user(uint8_t *dst, const void *src, size_t n) {
    size_t i;
    const uint8_t *s = (const uint8_t *)src;
    for (i = 0; i < n; i++) {
        dst[i] = s[i];
    }
}

static int mcs_can_read(uint32_t flags) {
    return (flags & MCS_O_RDONLY) != 0u || (flags & MCS_O_RDWR) != 0u;
}

static int mcs_can_write(uint32_t flags) {
    return (flags & MCS_O_WRONLY) != 0u || (flags & MCS_O_RDWR) != 0u;
}

void mcs_fd_table_init(mcs_fd_table_t *table) {
    size_t i;
    if (!table) {
        return;
    }
    for (i = 0; i < MCS_MAX_OPEN_FILES; i++) {
        table->files[i].used = 0u;
        table->files[i].flags = 0u;
        table->files[i].offset = 0u;
        table->files[i].node = (mcs_vnode_t *)0;
        table->files[i].fs = (mcs_ramfs_t *)0;
    }
}

static mcs_file_t *mcs_fd_get(mcs_fd_table_t *table, int fd) {
    if (!table || fd < 0 || (size_t)fd >= MCS_MAX_OPEN_FILES) {
        return (mcs_file_t *)0;
    }
    if (!table->files[fd].used) {
        return (mcs_file_t *)0;
    }
    return &table->files[fd];
}

static int mcs_fd_alloc(mcs_fd_table_t *table) {
    size_t i;
    if (!table) {
        return MCS_EINVAL;
    }
    for (i = 0; i < MCS_MAX_OPEN_FILES; i++) {
        if (!table->files[i].used) {
            table->files[i].used = 1u;
            return (int)i;
        }
    }
    return MCS_ENFILE;
}

int mcs_vfs_open(mcs_fd_table_t *table, mcs_ramfs_t *fs, const char *path, uint32_t flags) {
    mcs_vnode_t *node;
    int fd;
    int rc;
    if (!table || !fs || !path) {
        return MCS_EINVAL;
    }
    if ((flags & (MCS_O_RDONLY | MCS_O_WRONLY | MCS_O_RDWR)) == 0u) {
        flags |= MCS_O_RDONLY;
    }
    rc = mcs_ramfs_lookup(fs, path, &node);
    if (rc != MCS_OK) {
        if ((flags & MCS_O_CREAT) == 0u) {
            return rc;
        }
        rc = mcs_ramfs_create_file(fs, path, &node);
        if (rc != MCS_OK) {
            return rc;
        }
    }
    if (node->type == MCS_VNODE_DIR && mcs_can_write(flags)) {
        return MCS_EISDIR;
    }
    fd = mcs_fd_alloc(table);
    if (fd < 0) {
        return fd;
    }
    table->files[fd].flags = flags;
    table->files[fd].node = node;
    table->files[fd].fs = fs;
    table->files[fd].offset = ((flags & MCS_O_APPEND) != 0u) ? node->size : 0u;
    if ((flags & MCS_O_TRUNC) != 0u) {
        if (!mcs_can_write(flags)) {
            table->files[fd].used = 0u;
            table->files[fd].node = (mcs_vnode_t *)0;
            table->files[fd].fs = (mcs_ramfs_t *)0;
            return MCS_EACCES;
        }
        node->size = 0u;
        table->files[fd].offset = 0u;
    }
    return fd;
}

mcs_ssize_t mcs_vfs_read(mcs_fd_table_t *table, int fd, void *buf, size_t len) {
    mcs_file_t *file;
    size_t remain;
    size_t n;
    if (!buf && len != 0u) {
        return MCS_EINVAL;
    }
    file = mcs_fd_get(table, fd);
    if (!file) {
        return MCS_EBADF;
    }
    if (!mcs_can_read(file->flags)) {
        return MCS_EACCES;
    }
    if (!file->node || !file->fs) {
        return MCS_EINVAL;
    }
    if (file->node->type == MCS_VNODE_DIR) {
        return MCS_EISDIR;
    }
    if (file->offset >= file->node->size) {
        return 0;
    }
    remain = file->node->size - file->offset;
    n = mcs_min_size(len, remain);
    mcs_copy_to_user(buf, &file->fs->data[file->node->data_offset + file->offset], n);
    file->offset += n;
    return (mcs_ssize_t)n;
}

mcs_ssize_t mcs_vfs_write(mcs_fd_table_t *table, int fd, const void *buf, size_t len) {
    mcs_file_t *file;
    size_t n;
    if (!buf && len != 0u) {
        return MCS_EINVAL;
    }
    file = mcs_fd_get(table, fd);
    if (!file) {
        return MCS_EBADF;
    }
    if (!mcs_can_write(file->flags)) {
        return MCS_EACCES;
    }
    if (!file->node || !file->fs) {
        return MCS_EINVAL;
    }
    if (file->node->type == MCS_VNODE_DIR) {
        return MCS_EISDIR;
    }
    if ((file->flags & MCS_O_APPEND) != 0u) {
        file->offset = file->node->size;
    }
    if (file->offset > file->node->data_capacity) {
        return MCS_EINVAL;
    }
    n = mcs_min_size(len, file->node->data_capacity - file->offset);
    if (n < len) {
        return MCS_ENOSPC;
    }
    mcs_copy_from_user(&file->fs->data[file->node->data_offset + file->offset], buf, n);
    file->offset += n;
    if (file->offset > file->node->size) {
        file->node->size = file->offset;
    }
    return (mcs_ssize_t)n;
}

int mcs_vfs_lseek(mcs_fd_table_t *table, int fd, long offset, int whence) {
    mcs_file_t *file;
    long base;
    long next;
    file = mcs_fd_get(table, fd);
    if (!file) {
        return MCS_EBADF;
    }
    if (!file->node || file->node->type == MCS_VNODE_DIR) {
        return MCS_EISDIR;
    }
    if (whence == MCS_SEEK_SET) {
        base = 0;
    } else if (whence == MCS_SEEK_CUR) {
        base = (long)file->offset;
    } else if (whence == MCS_SEEK_END) {
        base = (long)file->node->size;
    } else {
        return MCS_EINVAL;
    }
    next = base + offset;
    if (next < 0) {
        return MCS_EINVAL;
    }
    file->offset = (size_t)next;
    return (int)file->offset;
}

int mcs_vfs_close(mcs_fd_table_t *table, int fd) {
    mcs_file_t *file = mcs_fd_get(table, fd);
    if (!file) {
        return MCS_EBADF;
    }
    file->used = 0u;
    file->flags = 0u;
    file->offset = 0u;
    file->node = (mcs_vnode_t *)0;
    file->fs = (mcs_ramfs_t *)0;
    return MCS_OK;
}

int mcs_vfs_dup(mcs_fd_table_t *table, int fd) {
    mcs_file_t *file;
    int newfd;
    file = mcs_fd_get(table, fd);
    if (!file) {
        return MCS_EBADF;
    }
    newfd = mcs_fd_alloc(table);
    if (newfd < 0) {
        return newfd;
    }
    table->files[newfd].flags = file->flags;
    table->files[newfd].offset = file->offset;
    table->files[newfd].node = file->node;
    table->files[newfd].fs = file->fs;
    return newfd;
}

int mcs_sys_open(mcs_process_t *proc, mcs_ramfs_t *fs, const char *user_path, uint32_t flags) {
    if (!proc || !fs || !user_path) {
        return MCS_EINVAL;
    }
    return mcs_vfs_open(&proc->fd_table, fs, user_path, flags);
}

mcs_ssize_t mcs_sys_read(mcs_process_t *proc, int fd, void *user_buf, size_t len) {
    if (!proc || (!user_buf && len != 0u)) {
        return MCS_EINVAL;
    }
    return mcs_vfs_read(&proc->fd_table, fd, user_buf, len);
}

mcs_ssize_t mcs_sys_write(mcs_process_t *proc, int fd, const void *user_buf, size_t len) {
    if (!proc || (!user_buf && len != 0u)) {
        return MCS_EINVAL;
    }
    return mcs_vfs_write(&proc->fd_table, fd, user_buf, len);
}

int mcs_sys_close(mcs_process_t *proc, int fd) {
    if (!proc) {
        return MCS_EINVAL;
    }
    return mcs_vfs_close(&proc->fd_table, fd);
}

int mcs_sys_lseek(mcs_process_t *proc, int fd, long offset, int whence) {
    if (!proc) {
        return MCS_EINVAL;
    }
    return mcs_vfs_lseek(&proc->fd_table, fd, offset, whence);
}
