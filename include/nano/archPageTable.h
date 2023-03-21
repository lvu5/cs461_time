#ifndef _ARCH_PAGE_TABLE_H_
#define _ARCH_PAGE_TABLE_H_

#if defined(__x86_32__)
#include <nano/x86_32/archPageTable.h>
#elif defined(__x86_64__)
#include <nano/x86_64/archPageTable.h>
#else
#error Unsupported architecture.
#endif

#define PT_PAGES           (1UL << PT_PAGE_ORDER)  // number of root pages
#define PT_ROOT_ENTRIES    (PT_USER_ENTRIES + PT_KERNEL_ENTRIES)
#define USER_BASEPTR(x)    ((ptentry_t *)(x) + PT_PAGE_ORDER * PT_ROOT_ENTRIES)

ptentry_t  *archPtpAlloc(void);

void        archPageTablePrintUserspace(pt_t pt);
void        archPageTableSwitch(pt_t pgd);
void        archPageTableLoad(void);
ptentry_t  *archPageTableInsert(pt_t,  vaddr_t, maddr_t, permission_t);
void        archPageTableReplace(pt_t,  vaddr_t, maddr_t, permission_t);
void        archPageTableRemove(pt_t, vaddr_t);
void        archPageTableFreeTree(pt_t pt, vaddr_t fromAddr, vaddr_t toAddr);
paddr_t     archPageTableToPaddr(pt_t pt, vaddr_t addr);
mfn_t       archPageTableMfn(pt_t pt, vaddr_t vaddr);

void        archPageTablePageUpdate(pt_t         pt,
				    pfn_t       *pfnArray,
				    vaddr_t     *vaddrArray,
				    ulong        count,
				    permission_t permission);
pt_t          archPageTableOfflineClone(pt_t new, pt_t old);
void          archPageTableOfflineAllocTree(pt_t pt, vaddr_t fromAddr, vaddr_t toAddr);
void          archPageTableOfflineUserInsert(pt_t pt, vaddr_t vaddr, maddr_t maddr, permission_t permission);
void          archPageTableOfflineCompleteNew(pt_t pt);
void          archPageTableOfflineCompleteExisting(pt_t pt);

void          archPageTableUserspaceCopy(pt_t pt, vaddr_t addr, vaddr_t hi, pt_t fromPt, vaddr_t offset, permission_t permission);

void          archPageTableFree(pt_t old);
void          archPageTableCopyRegion(pt_t, vaddr_t, vaddr_t, pt_t, vaddr_t, permission_t, int);
void          archPageTablePopulate(pfn_t *min, pfn_t *maxMapped, pfn_t *max);
void          archPageTablePropagateKernelEntry(pt_t current_pt, vaddr_t faulting_address);
vaddr_t       archPageTableNext(pt_t, vaddr_t, vaddr_t);
void          archPageTableWalk(vaddr_t vaddr);

ptentry_t    *archPteGet(pt_t pageTable, vaddr_t addr);
permission_t  archPteGetPermission(ptentry_t *p);
void          archPteUserUpdate(ptentry_t *p, permission_t ps);

void          archPageTableProtectRange(pt_t pt, vaddr_t startAddr, vaddr_t endAddr, permission_t permission);

void          archPageTableStatistics(pt_t pageTable);

#endif /* _ARCH_PAGE_H_ */
