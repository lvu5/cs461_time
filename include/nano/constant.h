//______________________________________________________________________________
/// Constants
//______________________________________________________________________________

#ifndef __CONSTANT_H__
#define __CONSTANT_H__
#include <nano/macros.h>

typedef enum ModeE {
	ReadMode=1,
	WriteMode=2,
	ExecuteMode=4
} Mode;

enum {
	MaxFileNameSize = 255,
	MaxUserNameSize = 255
};

enum {
	EthosRootDirectoryRd        = 1,
	EthosCurrentDirectoryRd     = 2,
	EthosEnvironmentDirectoryRd = 3,
	EthosArgumentDirectoryRd    = 4
};

#if defined(__x86_32__)
        // Heap region - 1GB at 1GB.
#define USERSPACE_HEAP_SIZE        ((vaddr_t)GB_TO_B(1))
#define USERSPACE_HEAP_START       ((vaddr_t)GB_TO_B(1))

#elif defined(__x86_64__)

       // Heap region - 1GB at 1TB.
#define USERSPACE_HEAP_SIZE        ((vaddr_t)(1UL<<40))
#define USERSPACE_HEAP_START       ((vaddr_t)GB_TO_B(1))

#else
#error Unsupported architecture.
#endif

#endif
