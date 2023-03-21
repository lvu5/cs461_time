#include <stddef.h>

char *my_strcat(char *dest, const char *src) {
    char *p = dest;
    while (*p) {
        p++;
    }
    while (*src) {
        *p++ = *src++;
    }
    *p = '\0';
    return dest;
}

char *my_strncat(char *dest, const char *src, size_t n) {
    char *p = dest;
    while (*p) {
        p++;
    }
    while (n--) {
        *p++ = *src++;
    }
    *p = '\0';
    return dest;
}

int my_strcmp(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        if (*s1 != *s2) {
            return *s1 - *s2;
        }
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

int my_strncmp(const char *s1, const char *s2, size_t n) {
    while (*s1 && *s2 && n--) {
        if (*s1 != *s2) {
            return *s1 - *s2;
        }
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

char *my_strcpy(char *dest, const char *src) {
    char *p = dest;
    while (*src) {
        *p++ = *src++;
    }
    *p = '\0';
    return dest;
}

char *my_strncpy(char *dest, const char *src, size_t n) {
    char *p = dest;
    while (n--) {
        *p++ = *src++;
    }
    *p = '\0';
    return dest;
}