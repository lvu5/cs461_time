//______________________________________________________________________________
// Xen-related low-level MMU updates.
//______________________________________________________________________________

#ifndef __XEN_PAGE_TABLE_H__
#define __XEN_PAGE_TABLE_H__

#include <nano/pageTable.h>
#include <nano/permission.h>

// pin a page table
void xenPageTablePin(pt_t pgd);

// unpin a page directory
void xenPageTableUnpin(pt_t pte);

#endif
