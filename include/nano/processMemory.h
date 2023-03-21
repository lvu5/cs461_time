//______________________________________________________________________________
// Ethos Process address space
// memory mapping function declarations
//
// Oct-2007: Satya Kiran Popuri
// Mar-2008: Andrei Warkentin
//______________________________________________________________________________

#ifndef __ETHOS_PROCESS_MEMORY_H__
#define __ETHOS_PROCESS_MEMORY_H__

#include <ethos/list.h>
#include <ethos/kernel/mm.h>
#include <ethos/kernel/mm_public.h>
#include <ethos/kernel/pageTable.h>

// Address space definition for a process.
struct ProcessMemory
{
  ListHead             regionList;     // Links together ProcessMemoryRegions.

  pt_t                 pageTable;     // Page table.

  ProcessMemoryRegion *stackRegion;  // The stack region.
};

// Print a processMemory
void processMemoryPrint(void *ptr);

#endif
