#ifndef __SLICE_H__
#define __SLICE_H__

#include <stdint.h>
#include <nano/ethosTypes.h>

typedef struct Slice_s {
	uchar *array;
	uint32_t typesize, len, cap;
} Slice;

Slice arraySlice(void *array, long typesize, long len, long cap);
Slice makeSlice(long typesize, long len, long cap);
Slice slice(Slice s, uint32_t i, uint32_t j);
void freeSlice(Slice s);
long len(Slice s);
long cap(Slice s);
//void *array(Slice s);
long typesize(Slice s);
void *element(Slice s, long i);
long copy(Slice d, Slice s);
long indexof(Slice s, unsigned c);
int slicecmp(Slice x, Slice y);

#define NULLSLICE ((Slice){NULL, 0, 0, 0})
#define ISNULLSLICE(x) ((x.array == NULL))

#endif
