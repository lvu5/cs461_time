//______________________________________________________________________________
// Public memory subsystem interfaces.
// Mar-2008: Andrei Warkentin
//______________________________________________________________________________

#ifndef __MM_PUBLIC_H__
#define __MM_PUBLIC_H__

#include <nano/permission.h>

// Forward declaration.
typedef struct ProcessMemory       ProcessMemory;
typedef struct ProcessMemoryRegion ProcessMemoryRegion;

//______________________________________________________________________________
// Process address space manipulation.
//______________________________________________________________________________

// Initialize processMemory* stuff for use.
void processMemoryInit(void);

// Create a new address space.
Status processMemoryNew(ProcessMemory **pmem);

// Destroy an address space.
void processMemoryFree(ProcessMemory *pmem);

// Switch to an address space.
void processMemorySwitch(const ProcessMemory *pmem);

// Clones the current memory context into a new one.
Status processMemoryClone(ProcessMemory **pmem, ProcessMemory *fromPmem);

// Creates stack and heap space
Status processMemoryStackHeapCreate(ProcessMemory *pmem);

//______________________________________________________________________________
// Process memory region manipulation.
//______________________________________________________________________________

// PMR are the process memory region creation flags.
// These are type flags and are exclusive.
#define MM_PMR_FREE_BACKED (0x1)    
                                    // Backed by newly-allocated free memory.
#define MM_PMR_SHADOWED    (0x2)    
                                    // Backed by same physical memory backing another PMR.

// Type mask.
#define MM_PMR_TYPE_MASK   (MM_PMR_FREE_BACKED | MM_PMR_SHADOWED)

// More PMR region creation flags.
#define MM_PMR_STACK       (0x2000)
                                     // PMR region is a stack region.
#define MM_PMR_COW         (0x8000)  
                                     // PMR is copy-on-write.
#define MM_PMR_LAZY        (0x10000) 
                                     // Do not immediately back pages.

// Other flags mask.
#define MM_PMR_OTHER_MASK  (MM_PMR_STACK | MM_PMR_COW | MM_PMR_LAZY)

// Allocate a memory region.
Status processMemoryRegionNewShadowed(
				      ProcessMemoryRegion **region,
				      ProcessMemory *pmem,
				      permission_t permission,
				      unsigned flags,
				      vaddr_t fixed_address,
				      ProcessMemoryRegion *sourceRegion
				      );

Status processMemoryRegionNewFreeBacked(
					ProcessMemoryRegion **region, 
					ProcessMemory *pmem, 
					permission_t prot, 
					unsigned flags, 
					vaddr_t fixed_address, 
					msize_t length
					);


void processMemoryRegionRemove(ProcessMemoryRegion *region);

// Sets different protections on a memory region.
Status processMemoryRegionProtect(
				  ProcessMemoryRegion *region,
				  permission_t prot
				  );

// Returns a ProcessMemoryRegion corresponding to region in address space
// mm containing address va.
Status processMemoryRegionFind(ProcessMemoryRegion **region, ProcessMemory *pmem, vaddr_t va);

//______________________________________________________________________________
// Virtual address
//______________________________________________________________________________

// Given an address, returns StatusOk if accesses at that address would not
// result in a page fault.
Status vaddrIsMapped(vaddr_t address);

// Returns StatusOk if the entire range from [address, address + length)
// is accessible without page faults.
Status vaddrRangeIsMapped(vaddr_t address, msize_t length);

#endif
