#include "mcs_vfs.h"

mcs_ramfs_t *mcs_active_ramfs_for_test = (mcs_ramfs_t *)0;

void mcs_vfs_set_active_ramfs_for_test(mcs_ramfs_t *fs) {
    mcs_active_ramfs_for_test = fs;
}
