#include "mcs_vfs.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void mcs_vfs_set_active_ramfs_for_test(mcs_ramfs_t *fs);

static void test_basic_read(void) {
    mcs_ramfs_t fs;
    mcs_process_t proc;
    char buf[32];
    int fd;
    mcs_ssize_t n;
    mcs_ramfs_init(&fs);
    assert(mcs_ramfs_seed_file(&fs, "/hello.txt", (const uint8_t *)"hello-mcsos", 11) == MCS_OK);
    proc.pid = 1;
    mcs_fd_table_init(&proc.fd_table);
    mcs_vfs_set_active_ramfs_for_test(&fs);
    fd = mcs_sys_open(&proc, &fs, "/hello.txt", MCS_O_RDONLY);
    assert(fd >= 0);
    memset(buf, 0, sizeof(buf));
    n = mcs_sys_read(&proc, fd, buf, 5);
    assert(n == 5);
    assert(memcmp(buf, "hello", 5) == 0);
    assert(mcs_sys_lseek(&proc, fd, 1, MCS_SEEK_SET) == 1);
    memset(buf, 0, sizeof(buf));
    n = mcs_sys_read(&proc, fd, buf, 4);
    assert(n == 4);
    assert(memcmp(buf, "ello", 4) == 0);
    assert(mcs_sys_close(&proc, fd) == MCS_OK);
    assert(mcs_sys_read(&proc, fd, buf, 1) == MCS_EBADF);
}

static void test_create_write_read(void) {
    mcs_ramfs_t fs;
    mcs_process_t proc;
    char buf[64];
    int fd;
    mcs_ssize_t n;
    mcs_ramfs_init(&fs);
    proc.pid = 2;
    mcs_fd_table_init(&proc.fd_table);
    mcs_vfs_set_active_ramfs_for_test(&fs);
    fd = mcs_sys_open(&proc, &fs, "/log.txt", MCS_O_CREAT | MCS_O_RDWR | MCS_O_TRUNC);
    assert(fd >= 0);
    n = mcs_sys_write(&proc, fd, "abc123", 6);
    assert(n == 6);
    assert(mcs_sys_lseek(&proc, fd, 0, MCS_SEEK_SET) == 0);
    memset(buf, 0, sizeof(buf));
    n = mcs_sys_read(&proc, fd, buf, sizeof(buf));
    assert(n == 6);
    assert(strcmp(buf, "abc123") == 0);
    assert(mcs_sys_close(&proc, fd) == MCS_OK);
}

static void test_errors_and_fd_limit(void) {
    mcs_ramfs_t fs;
    mcs_process_t proc;
    int fds[MCS_MAX_OPEN_FILES];
    size_t i;
    mcs_ramfs_init(&fs);
    assert(mcs_ramfs_seed_file(&fs, "/x", (const uint8_t *)"x", 1) == MCS_OK);
    proc.pid = 3;
    mcs_fd_table_init(&proc.fd_table);
    mcs_vfs_set_active_ramfs_for_test(&fs);
    assert(mcs_sys_open(&proc, &fs, "relative", MCS_O_RDONLY) == MCS_EINVAL);
    assert(mcs_sys_open(&proc, &fs, "/missing", MCS_O_RDONLY) == MCS_ENOENT);
    for (i = 0; i < MCS_MAX_OPEN_FILES; i++) {
        fds[i] = mcs_sys_open(&proc, &fs, "/x", MCS_O_RDONLY);
        assert(fds[i] == (int)i);
    }
    assert(mcs_sys_open(&proc, &fs, "/x", MCS_O_RDONLY) == MCS_ENFILE);
    assert(mcs_sys_close(&proc, fds[0]) == MCS_OK);
    assert(mcs_sys_open(&proc, &fs, "/x", MCS_O_RDONLY) == 0);
}

int main(void) {
    test_basic_read();
    test_create_write_read();
    test_errors_and_fd_limit();
    puts("M13 VFS/FD/RAMFS host tests: PASS");
    return 0;
}
