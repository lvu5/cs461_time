///
/// @file
///
/// \brief Standard library
///
/// A series of standard library routines.
/// June 2008, David Thulson

#ifndef __ECORE_H__
#define __ECORE_H__

#include <stdint.h>
#include <stdbool.h>

#define ASSERT_OR(x, y) do {if (!(x)) { y; }} while (0);

#ifndef NULL
#define NULL ((void*)0)
#endif

enum {
	stdinFd = 0,
	stdoutFd = 1,
	stderrFd = 2,
	rootDirectoryFd = 3,
	currentDirectoryFd = 4,
	environmentDirectoryFd = 5,
	argumentDirectoryFd = 6
};

#ifdef __SIZE_TYPE__ // Set by glibc, so may be available if building for POSIX (e.g., for testing)
typedef __SIZE_TYPE__ size_t;
#else
typedef unsigned long size_t;
#endif
/*typedef bool unsigned;
#define true 1
#define false 0*/

// Semantics for fill:
// - Copy data into buffer & update length (not position) using
//   a slice as a hint on how much to read.

// Semantics for drain:
// - Make space in the buffer & update length (not position) 
//   using a slice as a hint on how much space is needed.

typedef enum {
	BEGIN,
	CURRENT,
	END
} Whence;

// Dynamic memory allocation
void *malloc(size_t size);
void free(void *p);
void *realloc(void *p, size_t size);
void abort(void);

// Memory manipulation
/// \brief compares the first n bytes of the memory areas cs and ct
///
/// @param cs a pointer to a block of memory
/// @param ct a pointer to a block of memory
/// @param count the length of memory to compare
///
/// @return an integer less than, equal to, or greater than zero if the
/// first n bytes of cs is found, respectively, to be less than, to match,
/// or be greater than the first n bytes of ct
int memcmp(const void *cs, const void *ct, size_t count);
void *memccpy(void *dest, const void *src, int c, size_t count);
void *memmove(void *dest, const void *src, size_t count);
void *memcpy(void *dst, void const *src, size_t len);
void *memset(void *s, int c, size_t count);
void *memzero(void *s, size_t count);

static inline
bool
memAligned(void const *ptr)
{
    return !(((long) ptr) & (sizeof(long)-1));
}

#endif
