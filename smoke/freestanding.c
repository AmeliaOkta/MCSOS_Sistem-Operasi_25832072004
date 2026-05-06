#include <stdint.h>
#include <stddef.h>

#define MCSOS_M0_MAGIC 0x4D435330u /* "MCS0" */

struct m0_smoke_record {
    uint32_t magic;
    uint32_t version;
    uintptr_t pointer_width;
    size_t size_width;
};

__attribute__((used))
const struct m0_smoke_record m0_smoke_record = {
    .magic = MCSOS_M0_MAGIC,
    .version = 260502u,
    .pointer_width = sizeof(void *),
    .size_width = sizeof(size_t),
};

int m0_smoke_add(int a, int b) {
    return a + b;
}
