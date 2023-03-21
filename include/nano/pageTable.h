//______________________________________________________________________________
/// code is for manipulating globally pagetable mappings.
//______________________________________________________________________________

#ifndef __ETHOS_PAGE_TABLE_H__
#define __ETHOS_PAGE_TABLE_H__

#include <nano/mm.h>
#include <nano/permission.h>

pt_t currentPt;

// allocate a new page table.
pt_t pageTableAlloc(void);

// free a page table.
void pageTableFree(pt_t tab);

bool pageTablePfnUserFree(pfn_t pfn);

void pageTableUserProtectRange(pt_t, vaddr_t, vaddr_t, permission_t);

Status pageTableUserAlloc(pt_t, vaddr_t, permission_t);

Status pageTableUserCow(pt_t, vaddr_t);

Status pageTableKernelUserCow(pt_t, vaddr_t);

Status pageTableUserspaceAlloc(pt_t, vaddr_t, vaddr_t, permission_t);

void pageTableUserspaceCopy(pt_t    fromPt,
			    vaddr_t fromStart,
			    vaddr_t fromEnd,
			    pt_t    toPt, 
			    vaddr_t toStart, 
			    permission_t permission,
			    bool    isCow);

#endif /* __ETHOS_PAGE_TABLE_H__ */
