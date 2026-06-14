#ifndef MCSOS_SYSCALL_H
#define MCSOS_SYSCALL_H

#include <stdint.h>
#include <stddef.h>

#define MCSOS_SYSCALL_ABI_VERSION 1u
#define MCSOS_SYSCALL_MAX_ARGS 6u

typedef enum mcsos_syscall_nr {
    MCSOS_SYS_PING = 0,
    MCSOS_SYS_GET_TICKS = 1,
    MCSOS_SYS_WRITE_SERIAL = 2,
    MCSOS_SYS_YIELD = 3,
    MCSOS_SYS_EXIT_THREAD = 4,
    MCSOS_SYS_MAX = 5
} mcsos_syscall_nr_t;

typedef enum mcsos_syscall_status {
    MCSOS_OK = 0,
    MCSOS_EINVAL = -22,
    MCSOS_ENOSYS = -38,
    MCSOS_EFAULT = -14,
    MCSOS_EPERM = -1,
    MCSOS_EBUSY = -16
} mcsos_syscall_status_t;

typedef struct mcsos_syscall_frame {
    uint64_t nr;
    uint64_t arg0;
    uint64_t arg1;
    uint64_t arg2;
    uint64_t arg3;
    uint64_t arg4;
    uint64_t arg5;
    int64_t  ret;
} mcsos_syscall_frame_t;

typedef struct mcsos_user_region {
    uintptr_t base;
    uintptr_t limit;
} mcsos_user_region_t;

typedef struct mcsos_syscall_ops {
    uint64_t (*get_ticks)(void);
    void (*yield_current)(void);
    void (*exit_current)(int code);
    int64_t (*write_serial)(const char *buf, size_t len);
} mcsos_syscall_ops_t;

void mcsos_syscall_init(const mcsos_syscall_ops_t *ops);
void mcsos_syscall_set_user_region(mcsos_user_region_t region);
int mcsos_user_check_range(uintptr_t addr, size_t len);
int mcsos_copy_from_user(void *dst, const void *src, size_t len);
int64_t mcsos_syscall_dispatch(uint64_t nr, uint64_t arg0, uint64_t arg1,
                               uint64_t arg2, uint64_t arg3, uint64_t arg4,
                               uint64_t arg5);
void mcsos_syscall_dispatch_frame(mcsos_syscall_frame_t *frame);

#endif
