// declarations for all functions in mem.c, str.c, and print.c
#define _GNU_SOURCE

#ifndef MEMSTRPRINT_H
#define MEMSTRPRINT_H

#define printf my_printf

// mem.c
extern void *memcpy(void *dest, const void *src, size_t n);
extern void *memset(void *s, int c, size_t n);
extern int memcmp(const void *s1, const void *s2, size_t n);

// str.c
extern char *strcat(char *dest, const char *src);
extern char *strncat(char *dest, const char *src, size_t n);
extern int strcmp(const char *s1, const char *s2);
extern int strncmp(const char *s1, const char *s2, size_t n);
extern char *strcpy(char *dest, const char *src);
extern char *strncpy(char *dest, const char *src, size_t n);

//printf.c
extern void my_printf(char *fmt, ...);
#endif

