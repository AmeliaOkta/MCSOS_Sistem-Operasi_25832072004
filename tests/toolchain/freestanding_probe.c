#include <stdint.h>
#include <stddef.h>

volatile uint64_t mcsos_probe_sink;

static uint64_t rotl64(uint64_t x, unsigned int r) {
    return (x << r) | (x >> (64U - r));
}

uint64_t mcsos_toolchain_probe(uint64_t seed) {
    /* Kita pecah angkanya jadi dua bagian supaya compiler lama/strict tidak error */
    uint64_t magic = (0x4d43534fULL << 32) | 0x5332363035ULL;
    uint64_t x = seed ^ magic;
    
    for (size_t i = 0; i < 16; ++i) {
        x ^= (uint64_t)i * 0x9e3779b97f4a7c15ULL;
        x = rotl64(x, 13);
    }
    mcsos_probe_sink = x;
    return x;
}
