#define _GNU_SOURCE
#include <stddef.h>

void *my_memcpy(void *dest, const void *src, size_t n) {
    char *p = dest;
    const char *q = src;
    while (n--) {
        *p++ = *q++;
    }
    return dest;
}

void *my_memset(void *s, int c, size_t n) {
    char *p = s;
    while (n--) {
        *p++ = c;
    }
    return s;
}

int my_memcmp(const void *s1, const void *s2, size_t n) {
    const char *p1 = s1;
    const char *p2 = s2;
    while (n--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    return 0;
}
