//______________________________________________________________________________
/// ProcessMemoryRegion structure.
// Mar-2008: Andrei Warkentin
//______________________________________________________________________________

#ifndef __ETHOS_PROCESS_MEMORY_REGION_H__
#define __ETHOS_PROCESS_MEMORY_REGION_H__

#include <ethos/list.h>
#include <ethos/kernel/permission.h>
#include <ethos/kernel/processMemory.h>
#include <ethos/kernel/mm.h>

// Region pagefault handler for not-present faults.
typedef int (*RegionNotPresentHandler)(ProcessMemoryRegion *this, vaddr_t faulting_address);

// Region pagefault handler for access permission faults.
typedef int (*RegionBadAccessHandler)(ProcessMemoryRegion *this, vaddr_t faulting_address, permission_t tried);

struct ProcessMemoryRegion
{
    vaddr_t                 vm_start;
    vaddr_t                 vm_end;  
    ulong                   vm_flags;
    permission_t            vm_page_prot;
  
    // Call back for handling a page-not-present page fault.
    RegionNotPresentHandler not_present_pf;

    // Call back for handling a invalid access to present page fault.
    RegionBadAccessHandler  bad_access_pf;

    // For insertion into mm.
    ListHead list;

    // Type specific context...
    ProcessMemory *processMemory;
};

void processMemoryRegionInit(void);

// Print a processMemoryRegion
void processMemoryRegionPrint(void *ptr);

#endif 
