//______________________________________________________________________________
/// Low-level process management bits.
// Mar-2008: Andrei Warkentin
//______________________________________________________________________________

#ifndef __ARCH_SCHED_PRIVILEGED_H__
#define __ARCH_SCHED_PRIVILEGED_H__

#include <nano/cpuPrivileged.h>
//#include <nano/sched.h>
#include <nano/ethosTypes.h>

// order in bytes
#define KERNEL_STACK_ORDER      (PAGE_SHIFT+1)

// convert to bytes
#define KERNEL_STACK_SIZE     	(1<<KERNEL_STACK_ORDER)

// Kernel stack is two pages.
#define KERNEL_STACK_SIZE_IN_PAGES (KERNEL_STACK_SIZE/PAGE_SIZE)

// Kernel stack in terms of 2^ order (for use in pageAllocator).
#define KERNEL_STACK_SIZE_ORDER    (1)

typedef struct Process Process;

// Block kernel until events arrive and get served.
void archKernelBlock(void);


#endif /* __ARCH_SCHED_PRIVILEGED_H__ */
