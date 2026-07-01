#include "mcsos/syscall.h"

static mcsos_syscall_ops_t g_ops;
static mcsos_user_region_t g_user_region;

static int64_t default_write_serial(const char *buf, size_t len) {
    (void)buf;
    return (int64_t)len;
}

void mcsos_syscall_init(const mcsos_syscall_ops_t *ops) {
    g_ops.get_ticks = 0;
    g_ops.yield_current = 0;
    g_ops.exit_current = 0;
    g_ops.write_serial = default_write_serial;
    g_ops.get_current_fd_table = 0;
    g_ops.get_ramfs = 0;
    if (ops != 0) {
        if (ops->get_ticks != 0) g_ops.get_ticks = ops->get_ticks;
        if (ops->yield_current != 0) g_ops.yield_current = ops->yield_current;
        if (ops->exit_current != 0) g_ops.exit_current = ops->exit_current;
        if (ops->write_serial != 0) g_ops.write_serial = ops->write_serial;
        if (ops->get_current_fd_table != 0) g_ops.get_current_fd_table = ops->get_current_fd_table;
        if (ops->get_ramfs != 0) g_ops.get_ramfs = ops->get_ramfs;
    }
}

void mcsos_syscall_set_user_region(mcsos_user_region_t region) {
    g_user_region = region;
}

int mcsos_user_check_range(uintptr_t addr, size_t len) {
    if (len == 0u) return 1;
    if (g_user_region.base == 0u || g_user_region.limit <= g_user_region.base) return 0;
    if (addr < g_user_region.base) return 0;
    if (addr > g_user_region.limit) return 0;
    uintptr_t last = addr + (uintptr_t)len - 1u;
    if (last < addr) return 0;
    if (last >= g_user_region.limit) return 0;
    return 1;
}

int mcsos_copy_from_user(void *dst, const void *src, size_t len) {
    if (len == 0u) return MCSOS_OK;
    if (dst == 0 || src == 0) return MCSOS_EINVAL;
    if (!mcsos_user_check_range((uintptr_t)src, len)) return MCSOS_EFAULT;
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    for (size_t i = 0; i < len; ++i) d[i] = s[i];
    return MCSOS_OK;
}

int mcsos_copy_to_user(void *dst, const void *src, size_t len) {
    if (len == 0u) return MCSOS_OK;
    if (dst == 0 || src == 0) return MCSOS_EINVAL;
    if (!mcsos_user_check_range((uintptr_t)dst, len)) return MCSOS_EFAULT;
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    for (size_t i = 0; i < len; ++i) d[i] = s[i];
    return MCSOS_OK;
}

static int64_t sys_ping(uint64_t a0, uint64_t a1, uint64_t a2,
                        uint64_t a3, uint64_t a4, uint64_t a5) {
    (void)a0; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    return 0x2605020AL;
}

static int64_t sys_get_ticks(uint64_t a0, uint64_t a1, uint64_t a2,
                             uint64_t a3, uint64_t a4, uint64_t a5) {
    (void)a0; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    if (g_ops.get_ticks == 0) return MCSOS_EBUSY;
    return (int64_t)g_ops.get_ticks();
}

static int64_t sys_write_serial(uint64_t ptr, uint64_t len, uint64_t a2,
                                uint64_t a3, uint64_t a4, uint64_t a5) {
    (void)a2; (void)a3; (void)a4; (void)a5;
    if (ptr == 0u) return MCSOS_EINVAL;
    if (len > 4096u) return MCSOS_EINVAL;
    if (!mcsos_user_check_range((uintptr_t)ptr, (size_t)len)) return MCSOS_EFAULT;
    return g_ops.write_serial((const char *)(uintptr_t)ptr, (size_t)len);
}

static int64_t sys_yield(uint64_t a0, uint64_t a1, uint64_t a2,
                         uint64_t a3, uint64_t a4, uint64_t a5) {
    (void)a0; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    if (g_ops.yield_current == 0) return MCSOS_EBUSY;
    g_ops.yield_current();
    return MCSOS_OK;
}

static int64_t sys_exit_thread(uint64_t code, uint64_t a1, uint64_t a2,
                               uint64_t a3, uint64_t a4, uint64_t a5) {
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    if (g_ops.exit_current == 0) return MCSOS_EBUSY;
    g_ops.exit_current((int)code);
    return MCSOS_OK;
}


#define SYS_MAX_PATH 128u

static int64_t sys_open(uint64_t path_ptr, uint64_t path_len, uint64_t flags,
                        uint64_t a3, uint64_t a4, uint64_t a5) {
    (void)a3; (void)a4; (void)a5;
    if (g_ops.get_current_fd_table == 0 || g_ops.get_ramfs == 0) return MCSOS_EBUSY;
    if (path_len == 0u || path_len >= SYS_MAX_PATH) return MCSOS_EINVAL;
    char kpath[SYS_MAX_PATH];
    int rc = mcsos_copy_from_user(kpath, (const void *)(uintptr_t)path_ptr, (size_t)path_len);
    if (rc != MCSOS_OK) return rc;
    kpath[path_len] = '\0';
    mcs_fd_table_t *table = g_ops.get_current_fd_table();
    mcs_ramfs_t *fs = g_ops.get_ramfs();
    if (table == 0 || fs == 0) return MCSOS_EBUSY;
    return (int64_t)mcs_vfs_open(table, fs, kpath, (uint32_t)flags);
}

static int64_t sys_read(uint64_t fd, uint64_t buf_ptr, uint64_t len,
                        uint64_t a3, uint64_t a4, uint64_t a5) {
    (void)a3; (void)a4; (void)a5;
    if (g_ops.get_current_fd_table == 0) return MCSOS_EBUSY;
    if (len > 4096u) return MCSOS_EINVAL;
    unsigned char kbuf[4096];
    mcs_fd_table_t *table = g_ops.get_current_fd_table();
    if (table == 0) return MCSOS_EBUSY;
    mcs_ssize_t n = mcs_vfs_read(table, (int)fd, kbuf, (size_t)len);
    if (n < 0) return (int64_t)n;
    int rc = mcsos_copy_to_user((void *)(uintptr_t)buf_ptr, kbuf, (size_t)n);
    if (rc != MCSOS_OK) return rc;
    return (int64_t)n;
}

static int64_t sys_write(uint64_t fd, uint64_t buf_ptr, uint64_t len,
                         uint64_t a3, uint64_t a4, uint64_t a5) {
    (void)a3; (void)a4; (void)a5;
    if (g_ops.get_current_fd_table == 0) return MCSOS_EBUSY;
    if (len > 4096u) return MCSOS_EINVAL;
    unsigned char kbuf[4096];
    int rc = mcsos_copy_from_user(kbuf, (const void *)(uintptr_t)buf_ptr, (size_t)len);
    if (rc != MCSOS_OK) return rc;
    mcs_fd_table_t *table = g_ops.get_current_fd_table();
    if (table == 0) return MCSOS_EBUSY;
    return (int64_t)mcs_vfs_write(table, (int)fd, kbuf, (size_t)len);
}

static int64_t sys_lseek(uint64_t fd, uint64_t offset, uint64_t whence,
                         uint64_t a3, uint64_t a4, uint64_t a5) {
    (void)a3; (void)a4; (void)a5;
    if (g_ops.get_current_fd_table == 0) return MCSOS_EBUSY;
    mcs_fd_table_t *table = g_ops.get_current_fd_table();
    if (table == 0) return MCSOS_EBUSY;
    return (int64_t)mcs_vfs_lseek(table, (int)fd, (long)offset, (int)whence);
}

static int64_t sys_close(uint64_t fd, uint64_t a1, uint64_t a2,
                         uint64_t a3, uint64_t a4, uint64_t a5) {
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    if (g_ops.get_current_fd_table == 0) return MCSOS_EBUSY;
    mcs_fd_table_t *table = g_ops.get_current_fd_table();
    if (table == 0) return MCSOS_EBUSY;
    return (int64_t)mcs_vfs_close(table, (int)fd);
}

typedef int64_t (*syscall_fn_t)(uint64_t, uint64_t, uint64_t,
                                uint64_t, uint64_t, uint64_t);

static syscall_fn_t g_table[MCSOS_SYS_MAX] = {
    sys_ping,
    sys_get_ticks,
    sys_write_serial,
    sys_yield,
    sys_exit_thread,
    sys_open,
    sys_read,
    sys_write,
    sys_lseek,
    sys_close
};

int64_t mcsos_syscall_dispatch(uint64_t nr, uint64_t arg0, uint64_t arg1,
                               uint64_t arg2, uint64_t arg3, uint64_t arg4,
                               uint64_t arg5) {
    if (nr >= (uint64_t)MCSOS_SYS_MAX) return MCSOS_ENOSYS;
    syscall_fn_t fn = g_table[nr];
    if (fn == 0) return MCSOS_ENOSYS;
    return fn(arg0, arg1, arg2, arg3, arg4, arg5);
}

void mcsos_syscall_dispatch_frame(mcsos_syscall_frame_t *frame) {
    if (frame == 0) return;
    frame->ret = mcsos_syscall_dispatch(frame->nr, frame->arg0, frame->arg1,
                                        frame->arg2, frame->arg3, frame->arg4,
                                        frame->arg5);
}
