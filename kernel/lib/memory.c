#include <stddef.h>

void *memset(void *dest, int value, size_t count) {
    unsigned char *d = (unsigned char *)dest;
    while (count-- != 0u) {
        *d++ = (unsigned char)value;
    }
    return dest;
}

void *memcpy(void *dest, const void *src, size_t count) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    while (count-- != 0u) {
        *d++ = *s++;
    }
    return dest;
}

void *memmove(void *dest, const void *src, size_t count) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;

    if (d == s || count == 0u) {
        return dest;
    }

    if (d < s) {
        while (count-- != 0u) {
            *d++ = *s++;
        }
    } else {
        d += count;
        s += count;
        while (count-- != 0u) {
            *--d = *--s;
        }
    }

    return dest;
}
