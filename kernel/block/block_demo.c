#include "mcsos/block.h"
#include "mcsos/kernel/panic.h"

static uint8_t g_m14_ramdisk_storage[512u * 64u];
static mcsos_blk_device_t g_m14_ramdisk_dev;
static mcsos_ramblk_t g_m14_ramdisk;

void m14_block_demo_init(void) {
    mcsos_blk_registry_reset();

    mcsos_blk_status_t status = mcsos_ramblk_init(
        &g_m14_ramdisk_dev,
        &g_m14_ramdisk,
        "ram0",
        g_m14_ramdisk_storage,
        sizeof(g_m14_ramdisk_storage),
        512u);
    if (status != MCSOS_BLK_OK) {
        KERNEL_PANIC("m14_block_demo_init: ramblk_init failed", (uint64_t)status);
    }

    status = mcsos_blk_register(&g_m14_ramdisk_dev);
    if (status != MCSOS_BLK_OK) {
        KERNEL_PANIC("m14_block_demo_init: blk_register failed", (uint64_t)status);
    }
}
