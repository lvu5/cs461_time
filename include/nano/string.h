//______________________________________________________________________________
/// String manipulation functions
// Declarations copied from string.c (see string.c for copyright)
//
// June-2008: David Thulson
//______________________________________________________________________________

#ifndef _ETHOS_STRING_H_
#define _ETHOS_STRING_H_

#include <ethos/core.h>

unsigned long strtoul(const char *cp, char **endp, unsigned int base);
long strtol(const char *cp, char **endp, unsigned int base);
unsigned long long strtoull(const char *cp, char **endp, unsigned int base);
long long strtoll(const char *cp, char **endp, unsigned int base);

int strncmp(const char * cs, const char * ct, size_t count);
int strcmp(const char * cs, const char * ct);
char *strcpy(char * dest, const char *src);
char *strncpy(char * dest, const char *src, size_t count);
size_t strnlen(const char * s, size_t count);
char *strcat(char * dest, const char * src);
char *strncat(char * dest, const char * src, size_t count);
size_t strlen(const char * s);
char *strchr(const char * s, int c);
char* strrchr(const char *str, int c);
char* strnrchr(const char *s, int c, size_t len);
char* strchrnul(const char *str, int c);
char *strstr(const char * s1, const char * s2);
char *strdup(const char *s);

#endif
