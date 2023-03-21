//______________________________________________________________________________
///  Architecture specific page table code.
//
//  2007     (c) Satya Popuri
//  2008     (c) Andrei Watekin
//  Jun-2009 (c) Jon A. Solworth
//______________________________________________________________________________


#include <nano/common.h>
#include <nano/mm.h>
#include <nano/debug.h>
#include <nano/pageTable.h>
#include <nano/xenPageTable.h>
#include <nano/archPageTable.h>
#include <nano/memory.h>
#include <nano/physicalInfo.h>
#include <nano/fmt.h>

extern char stack[];
mfn_t *pfnToMfnArray;

#define MAX_PTS      4096
const int           mmu_update_size = MAX_PTS+2;
static mmu_update_t mmu_updates[MAX_PTS+2]; 
static int          mmu_update_count = 0;

const vaddr_t       MaxAddr = ~ (vaddr_t) 0;

static pfn_t        startPfn;      // starting pfn

void                pageTablePtpFree(void *page);
ptentry_t*          pageTablePtpAlloc(void);

static ptentry_t   *_offlinePtpAlloc();
static ptentry_t   *_insertInternal(pt_t pt, vaddr_t vaddr, maddr_t maddr, pteflags_t pteflags);
static void         _offlineFree(pt_t pt, vaddr_t fromAddr, vaddr_t toAddr);
static long         ptNoWriteCount; // number of pts created, waiting to remove write access
static vaddr_t      ptNoWrite[MAX_PTS];

#if defined(__x86_64__)
static void         _cloneUser(ptentry_t *ptePtr, ptentry_t pte);
static void         _offlineDuplicateL4(pt_t pt, uint offset);
#endif

typedef enum {
    I7_NONE,
    I7_INVLPG,
} I7handle;

static I7handle i7=I7_INVLPG;

//______________________________________________________________________________
///  do the deferred changes
//______________________________________________________________________________
static inline
void
_doUpdate(void)
{
    if (!mmu_update_count)  // nothing to do
	{
	    return;
	}

    // no need to pin here, only need to pin the top directories.
    int err = HYPERVISOR_mmu_update(mmu_updates, mmu_update_count, NULL, DOMID_SELF);
    if (err < 0) 
	{
	    xprintLog("ERROR: mmu_update failed with err = $[int]\n", err);
	    BUG();
	}
    mmu_update_count = 0;

    archPageTableLoad();
}

//______z________________________________________________________________________
/// deferred page table entries
//______________________________________________________________________________
static inline
void
_deferredUpdate(maddr_t maddr,      ///< machine address of pte
		ptentry_t pte       ///< new pte
		)
{
    if (mmu_update_count >= mmu_update_size)
	{
	    _doUpdate();
	}
    mmu_updates[mmu_update_count].ptr = maddr;
    mmu_updates[mmu_update_count].val = pte;
    mmu_update_count++;
}


//______________________________________________________________________________
/// creates a ptentry
//______________________________________________________________________________
static inline
ptentry_t
_pteCreate(maddr_t    maddr,              ///< the machine address or ptentry
	   pteflags_t pteflags            ///< the page table entry flags
	   )
{
    return (maddr & FRAME_MASK) | pteflags;
}

//______________________________________________________________________________
/// Compute page table (permission) flags from a permission_t.
//______________________________________________________________________________
static
pteflags_t
_pteFlags(permission_t permission, bool userSpace)
{
    pteflags_t pteFlags = 0;  
    if (userSpace)
	{
	    pteFlags |= _PAGE_USER;
	}
    if (permission == PERM_NONE)
	{
	    pteFlags |= _PAGE_PERM_NONE;
	    return pteFlags;
	}
    pteFlags |= _PAGE_PRESENT;
    if (permission & PERM_WRITE)
	{
	    pteFlags |= _PAGE_RW;
	} 
    return pteFlags;
}

//______________________________________________________________________________
/// Return pointer to page table entry matching vaddr, null if no page entry.
//______________________________________________________________________________
ptentry_t *
archPteGetPtr(pt_t pt, vaddr_t vaddr)
{
    ASSERT(pt);

    ptentry_t *ptePtr;

#ifdef HAS_L4
    ptePtr = pt + l4offset(vaddr);
    if (archPteIsFree(*ptePtr))
	return NULL;

    pt = (pt_t) pteToVirtual(*ptePtr);
#endif

    ptePtr = pt  + l3offset(vaddr);
    if (archPteIsFree(*ptePtr))
	return NULL;

    pt = (pt_t) pteToVirtual(*ptePtr);

    ptePtr = pt  + l2offset(vaddr);
    if (archPteIsFree(*ptePtr))
	return NULL;

    pt = (pt_t) pteToVirtual(*ptePtr);

    ptePtr = pt  + l1offset(vaddr);
  
    return ptePtr;
}

//______________________________________________________________________________
/// Return pointer to page table entry matching vaddr, null if no page entry
/// or page entry is not valid.
//______________________________________________________________________________
ptentry_t *
archPteGet(pt_t pt, vaddr_t vaddr)
{
    ptentry_t *ptePtr = archPteGetPtr(pt, vaddr);
    if (archPteIsFree(*ptePtr))
	return NULL;
  
    return ptePtr;
}


//______________________________________________________________________________
// get permission set of a page table entry 
//______________________________________________________________________________
permission_t
archPteGetPermission(ptentry_t *ptePtr)
{
    ASSERT(ptePtr);
    ASSERT(archPteInUse(*ptePtr));

    if (*ptePtr & _PAGE_PERM_NONE)
	{
	    return PERM_NONE;
	}
    else if (*ptePtr & _PAGE_RW)
	{
	    return PERM_READ | PERM_WRITE | PERM_EXEC;
	}
    return PERM_READ | PERM_EXEC;
}

//______________________________________________________________________________
// get the machine frame number corresponding to -vaddr- in page table -pt-
//______________________________________________________________________________
mfn_t
archPageTableMfn(pt_t    pt,
		 vaddr_t vaddr)
{
    ptentry_t *ptentry = archPteGet(pt, vaddr);
    ASSERT(ptentry);
    return pteToMfn(*ptentry);
}


//______________________________________________________________________________
/// initial page allocator, used for mapping the starting page table
/// called by pageTablePtpAlloc().
//______________________________________________________________________________
ptentry_t*
archPtpAlloc(void)
{
    // before physicalInfo is set up, do simple allocation
    // this suffices for mapping all the pages into memory
    // address space
    pfn_t   pfn   = startPfn++;
    vaddr_t vaddr = pfnToVirtual(pfn);
    return (ptentry_t *) vaddr;
}

//_____________________________________________________________________________
// computes deferred updates to both pte and page
//_____________________________________________________________________________
static inline
void
_ptpInternal(ptentry_t  *ptePtr,
		ptentry_t   pte,
		ptentry_t  *childPtePtr,
		ptentry_t   childPte
		)
{
    _deferredUpdate(virtualToMachine((vaddr_t) ptePtr), pte); // update ptp

    _deferredUpdate(virtualToMachine((vaddr_t) childPtePtr), childPte);
}

//_____________________________________________________________________________
// computes deferred updates for pte changes
//_____________________________________________________________________________
static inline
void
_ptpInternalUpdate(ptentry_t *ptePtr, ptentry_t pte)
{
    _deferredUpdate(virtualToMachine((vaddr_t) ptePtr), pte);
}

//_____________________________________________________________________________
// fix for Intel core i7 tlb changes.
// It invalidates the virtual address which contains modified ptps
//_____________________________________________________________________________
static inline
void
_ptpI7(vaddr_t vaddr)
{
    switch (i7)
	{
	case I7_NONE:
	    return;
	case I7_INVLPG:
	    {
		struct mmuext_op op[2];
		op[0].cmd = MMUEXT_INVLPG_LOCAL;
		op[0].arg1.linear_addr = (vaddr_t) vaddr;
		int reason  = HYPERVISOR_mmuext_op(op, 1, NULL, DOMID_SELF);
		if ( reason < 0) {
		    xprintLog("could invalidate, error = $[int]\n", reason);
		    BUG();
		}
	    }
	    return;
	}

}

//______________________________________________________________________________
/// Updates a valid user-space page table entry with new permission
//______________________________________________________________________________
void
archPteUserUpdate(ptentry_t *ptePtr, permission_t permission)
{
    ASSERT(ptePtr);
    pteflags_t pteflags = _pteFlags(permission, true);
    ptentry_t    newPte = _pteCreate(*ptePtr, pteflags);

    _ptpInternalUpdate(ptePtr, newPte);

    _doUpdate();

    // i7
    _ptpI7(pteToVirtual(newPte));
}


void
_rangeSet(
	  pt_t pt,
	  vaddr_t startAddr,
	  vaddr_t endAddr,
	  pteflags_t pteflags
	  );

//______________________________________________________________________________
/// change protection on a range of addresses
//______________________________________________________________________________
void
archPageTableProtectRange(
			  pt_t pt,
			  vaddr_t startAddr,
			  vaddr_t endAddr,
			  permission_t permission
			  )
{
    //ASSERT(isUserspaceAddress(endAddr));

    pteflags_t pteFlags = _pteFlags(permission, true);
    //xprintLog("archPageTableProtectRange:  permission =  $[xlong]\n", permission);

    _rangeSet(pt, startAddr, endAddr, pteFlags);

    _doUpdate();

    return;
}

//______________________________________________________________________________
// free a ptp.
//______________________________________________________________________________
static
void
_ptpFree(pt_t pt,
	 ptentry_t *ptePtr)
{
    ASSERT(ptePtr);
    ASSERT(archPteInUse(*ptePtr));
    
    ptentry_t pte = 0; // mark it as unused

    vaddr_t     child       = pteToVirtual(*ptePtr);

    // free up page table page
    pageTablePtpFree((void *) child);

    ptentry_t *childPtePtr = archPteGet(pt, child);

    _ptpInternal(ptePtr, pte, childPtePtr, *childPtePtr | _PAGE_RW);
}


//______________________________________________________________________________
// free up the root of the page table and user space
// NB. This code requires that all updates are deferred until the end
//______________________________________________________________________________
void
archPageTableFree(pt_t pt)
{
    // i7 issue

    _offlineFree(pt, USERSPACE_START, USERSPACE_END);

#if defined(__x86_32__)
    ptentry_t *pte = &pt[PT_USER_ENTRIES]; // first kernel entry
    void *l2KernelPage = (void *) pteToVirtual(*pte);
    ASSERT(l2KernelPage); // can't be NULL, then the kernel is not mapped
    _ptpFree(pt, pte);
#endif
    ptentry_t *childPtePtr = archPteGet(pt, (vaddr_t) pt);
    _ptpInternalUpdate(childPtePtr, *childPtePtr | _PAGE_RW);

#if defined(__x86_64__)
    childPtePtr = archPteGet(pt, (vaddr_t) USER_BASEPTR(pt));
    _ptpInternalUpdate(childPtePtr, *childPtePtr | _PAGE_RW);
#endif

    _doUpdate();
}    

//______________________________________________________________________________
// allocate if necessary and update the offline copy of the tree. 
//______________________________________________________________________________
static
bool
_offlineAllocTreePtp(ptentry_t *ptePtr,
		     pteflags_t  pteflags)
{
    if (archPteIsFree(*ptePtr))
	{
	    ptentry_t *childPtePtr = _offlinePtpAlloc();
	    if (!childPtePtr)
		{
		    return false;
		}
	    *ptePtr     = _pteCreate(virtualToMachine((vaddr_t) childPtePtr), pteflags);
	    ASSERT(archPteInUse(*ptePtr));
	}

    //xprintLog("$[str]: fromAddr=$[xlong]  toAddr=$[xlong]   virtual=$[xlong]  pte=$[xint64]\n",
    //   __func__, fromAddr, toAddr, pteToVirtual(*ptePtr), *ptePtr);

    return true;
}

//______________________________________________________________________________
// create a page table subtree at the third level, ptes inserted at 3rd level
//______________________________________________________________________________
static
bool
_offlineAllocTreeLevel2(pt_t       pt,
			ptentry_t *ptePtr,
			vaddr_t    fromAddr,
			vaddr_t    toAddr
			)
{

    if (!_offlineAllocTreePtp(ptePtr, L3_PROT))
	{
	    return false;
	}

    ptePtr  = (ptentry_t*) pteToVirtual(*ptePtr);

    //xprintLog("$[str]: fromAddr=$[xlong]  toAddr=$[xlong]   virtual=$[xlong]   pte=$[xint64]\n",
    //	   __func__, fromAddr, toAddr, ptePtr, *ptePtr);

    vaddr_t min = l2offset(fromAddr);
    vaddr_t max = l2offset(toAddr);
    ulong l;
    for (l=min; l <= max; l++)
	{
	    if (!_offlineAllocTreePtp(ptePtr + l, L2_PROT))
		{
		    return false;
		}
	}

    return true;
}


#ifdef HAS_L4
//______________________________________________________________________________
// create a page table subtree at the third level
//______________________________________________________________________________
static
bool
_offlineAllocTreeLevel3(pt_t       pt,
			ptentry_t *ptePtr,
			vaddr_t    fromAddr,
			vaddr_t    toAddr
			)
{
    if (!_offlineAllocTreePtp(ptePtr, L4_PROT))
	{
	    return false;
	}


    ptePtr  = (ptentry_t*) pteToVirtual(*ptePtr);

    vaddr_t min = l3offset(fromAddr);
    vaddr_t max = l3offset(toAddr);
    vaddr_t f   = fromAddr & L2_MASK;
    vaddr_t t   = L2_MASK;
    ulong l;
    for (l=min; l <= max; l++)
	{
	    if (l==max)
		{
		    t = toAddr & L2_MASK;
		}
	    if (!_offlineAllocTreeLevel2(pt, ptePtr + l, f, t))
		{
		    return false;
		}
	    f = 0;
	}

   return true;
}
#endif


//______________________________________________________________________________
/// Walks the userspace part [fromAddr, toAddr] of the multilevel page tree, 
/// adding pts to support the address space
//
// i7: this routine has a single update to deal with per ptp, and so is OK for i7
//______________________________________________________________________________
void
archPageTableOfflineAllocTree(pt_t pt, vaddr_t fromAddr, vaddr_t toAddr)
{
    ASSERT(toAddr <= USERSPACE_END);

    //xprintLog("$[str]: fromAddr=$[xlong]  toAddr=$[xlong]\n",
    //   __func__, fromAddr, toAddr);

#ifdef HAS_L4
    vaddr_t min = l4offset(fromAddr);
    vaddr_t max = l4offset(toAddr);
    vaddr_t f   = fromAddr & L3_MASK;
    vaddr_t t   = L3_MASK;
    //xprintLog("$[str]: f=$[xlong]  t=$[xlong]   min=$[xlong]  max=$[xlong]\n",
    //   __func__, fromAddr, toAddr, min, max);

    ulong l;
    for (l=min; l <= max; l++)
	{
	    if (l==max)
		{
		    t = toAddr & L3_MASK;
		}
	    bool result = _offlineAllocTreeLevel3(pt, pt + l, f, t);
	    ASSERT(result);
	    f = 0;
	}
#else
    vaddr_t min = l3offset(fromAddr);
    vaddr_t max = l3offset(toAddr);
    vaddr_t f   = fromAddr & L2_MASK;
    vaddr_t t   = L2_MASK;
    //xprintLog("$[str]: f=$[xlong]  t=$[xlong]   min=$[xlong]  max=$[xlong]\n",
    //   __func__, fromAddr, toAddr, min, max);

    ulong l;
    for (l=min; l <= max; l++)
	{
	    if (l==max)
		{
		    t = toAddr & L2_MASK;
		}
	    bool result = _offlineAllocTreeLevel2(pt, pt + l, f, t);
	    ASSERT(result);
	    f = 0;
	}
#endif
}


//______________________________________________________________________________
/// copies over the kernel pages table entries from the old page table to the new
/// page table, and zeroes out the user space entries.  newPt is *not* the current
//  page table pointer.
//______________________________________________________________________________
pt_t
archPageTableOfflineClone(pt_t newPt, pt_t oldPt)
{
    ASSERT(newPt);
    ASSERT(oldPt);

#if defined(__x86_32__)
    // In PAE mode, processes may not share L3 kernel entries.
    // So, create a new L2, copy the kernel's L2 contents into it and
    // add it to the process' L3 table.
    ptentry_t *oldPtp = (ptentry_t *) pteToVirtual(oldPt[PT_USER_ENTRIES]); // Entry 4 = ker. mem.
    ptentry_t *newPtp = _offlinePtpAlloc();
    BUG_ON(!newPtp);

    memcpy ((void *) newPtp, (void *) oldPtp, PAGE_SIZE);

    // Create new page table entry; match old page's control bits:
    ptentry_t newPte = _pteCreate(virtualToMachine((vaddr_t) newPtp), L3_PROT);
    ASSERT(!((newPte ^ oldPt[PT_USER_ENTRIES])&FRAME_INTERIOR_MASK));

    // Copy L3 entries corresponding to existing kernel mappings
    // from the master kernel tables while fulfilling Xen requirements. 
    newPt[PT_USER_ENTRIES] = newPte;

    // clear out the user entries in the root page of the page table
    memzero((void *) newPt, PT_USER_ENTRIES * sizeof(ptentry_t));

#else
    ptentry_t *newPtUser   = USER_BASEPTR(newPt);
    ptentry_t *newPtKernel = newPt;
    int i;
    // copy over the kernel entries of the pt
    for (i=0; i<PT_USER_ENTRIES; i++, newPtUser++,newPtKernel++)
	{
	    *newPtUser   = 0;
	    *newPtKernel = 0;
	}
    ptentry_t *oldPtPtr = oldPt + PT_USER_ENTRIES;
    for (i=0; i<PT_KERNEL_ENTRIES; i++, newPtUser++, newPtKernel++)
	{
	    *newPtUser   = 0;
	    *newPtKernel = *oldPtPtr;
	    oldPtPtr++;
	}
#endif

    return newPt;
}

#if defined(__x86_64)
//______________________________________________________________________________
// duplicate l4 entry for Xen's dual root pages in 64 bit
//______________________________________________________________________________
static inline
void
_offlineDuplicateL4(pt_t pt, uint offset)
{
    USER_BASEPTR(pt)[offset] = pt[offset];
}


//______________________________________________________________________________
/// Update an entry in the user page table to match the kernel page table
//______________________________________________________________________________
static
void
_cloneUser(ptentry_t *ptePtr, ptentry_t pte)
{
    ASSERT(ptePtr);

    _ptpInternalUpdate(ptePtr, pte);
}

#endif

//______________________________________________________________________________
// pt is a new page table, but we are not switching to it
// instead we remote write access and pin it.
//______________________________________________________________________________
void
archPageTableOfflineCompleteExisting(pt_t pt)
{
    //xprintLog("$[str]:   ptNoWriteCount=$[ulong]   pt=$[pointer]\n", __func__, ptNoWriteCount, pt);

    ptNoWrite[ptNoWriteCount++] = (vaddr_t) pt; // add the page table

#if defined(__x86_64__)
    ptNoWrite[ptNoWriteCount++] = (vaddr_t) USER_BASEPTR(pt); // add the user page table
#endif

    ulong i;
    for (i=0; i<ptNoWriteCount; i++)
	{
	    vaddr_t ptp = ptNoWrite[i];
	    //xprintLog("$[str]:   ptNoWrite[$[int]]=$[xlong]\n",  __func__, i, ptp);
	    ptentry_t *ptePtr = archPteGet((pt_t) start_info.pt_base, ptp);
	    ASSERT(ptePtr);
	    _ptpInternalUpdate(ptePtr, *ptePtr & ~_PAGE_RW);
	}

    _doUpdate();

    xenPageTablePin(pt);
#if defined(__x86_64__)
	// x86-64: need to pin the user page table as well
    xenPageTablePin(USER_BASEPTR(pt));
#endif
 
    ptNoWriteCount = 0;
}

//______________________________________________________________________________
/// ensure the offline updates, the PTP write removal are done.
/// result is that pt is the current page table
//______________________________________________________________________________
void
archPageTableOfflineCompleteNew(pt_t pt)
{
    archPageTableOfflineCompleteExisting(pt);

    archPageTableSwitch(pt);
}

//______________________________________________________________________________
// Insert a level of the tree, if necessary, otherwise traverse
//______________________________________________________________________________
static
ptentry_t*
_offlinePathLevel(ptentry_t  *ptePtr,
			      pteflags_t  pteFlags
			      )
{
    ASSERT(ptePtr);

    if (!archPteIsFree(*ptePtr))
	{    
	    return (ptentry_t *) pteToVirtual(*ptePtr);
	} 

    // need a page beneath this level
    ptentry_t *page = _offlinePtpAlloc();
    ASSERT(page);

    ptentry_t pte  = _pteCreate(virtualToMachine((vaddr_t) page), pteFlags);
	    
    *ptePtr = pte;

    return page;
}



//______________________________________________________________________________
// walk from pt to vaddr, creating ptps as necessary
//______________________________________________________________________________
static 
ptentry_t *
_offlinePath(pt_t    pt,          ///< page table 
	     vaddr_t vaddr        ///< the new virtual address
	     )
{
    ptentry_t *tab      = pt;

#ifdef HAS_L4
    // X86-64
    tab = _offlinePathLevel(tab + l4offset(vaddr), L4_PROT);
    // in case top level page table page has been updated
    _offlineDuplicateL4(pt, l4offset(vaddr));
#endif

    tab = _offlinePathLevel(tab + l3offset(vaddr),  L3_PROT);
  
    // all paged systems must have l2 and l1

    // the l2 level entries
    tab = _offlinePathLevel(tab + l2offset(vaddr), L2_PROT);

    ptentry_t *ptePtr = tab + l1offset(vaddr);

    return ptePtr;
}


//______________________________________________________________________________
/// offline ptp update
//______________________________________________________________________________
static
ptentry_t*
_offlinePtpAlloc(void)
{
    if (ptNoWriteCount+1 >= MAX_PTS)
	{
	    return 0;
	}
    ptentry_t *ptp = pageTablePtpAlloc();
    if (ptp)
	{
	    ptNoWrite[ptNoWriteCount++] = (vaddr_t) ptp;
	}
    return ptp;
}

//______________________________________________________________________________
/// offline page table insert of user pages.
// 
//  i7: offline, so no i7 issues
//______________________________________________________________________________
void
archPageTableOfflineUserInsert(pt_t         pt,
			       vaddr_t      vaddr,
			       maddr_t      maddr,
			       permission_t permission
			       )
{
    ASSERT((maddr & FRAME_MASK) == maddr);
    ptentry_t   *ptePtr = _offlinePath(pt, vaddr);
    ASSERT(ptePtr);

    pteflags_t pteFlags = _pteFlags(permission, true);
    ptentry_t pte = _pteCreate(maddr, pteFlags);
    //xprintLog("$[str]: vaddr=$[pointer]  $[xint64]\n", __func__, vaddr, pte);
    *ptePtr = pte;
}

//______________________________________________________________________________
/// Walks the userspace part [fromAddr, toAddr] of the multilevel page tree, 
/// removing unused levels below the pt
//______________________________________________________________________________
static
void
_offlineFree(pt_t pt, vaddr_t fromAddr, vaddr_t toAddr)
{
    ASSERT(toAddr <= USERSPACE_END);
    ptentry_t *l4ptr, *l3ptr, *l2ptr;

    ptentry_t pte = _pteCreate(virtualToMachine((vaddr_t) pt), 0);
    l4ptr = l3ptr = &pte;

#ifdef HAS_L4
    int l4;
    for (l4=0; l4<L4_USER_ENTRIES; l4++)
	{
	    l4ptr = pt + l4;
	    if (archPteIsFree(*l4ptr))
		continue;
#endif
	    ptentry_t *t3 = (ptentry_t *) pteToVirtual(*l4ptr);

	    int l3;
	    for (l3=0; l3<L3_USER_ENTRIES; l3++)
		{
		    l3ptr = t3 + l3;
		    if (archPteIsFree(*l3ptr))
			continue;

		    ptentry_t *t2 = (ptentry_t *) pteToVirtual(*l3ptr);

		    int l2;
		    for (l2=0; l2<L2_USER_ENTRIES; l2++) 
			{
			    l2ptr = t2 + l2;
			    if (archPteIsFree(*l2ptr)) 
				{
				    continue;
				}

			    ptentry_t *t1 = (ptentry_t *) pteToVirtual(*l2ptr);

			    int l1;
			    for (l1=0; l1 < L1_USER_ENTRIES; l1++)
				{
				    ptentry_t pte = t1[l1];
				    if (!archPteIsFree(pte))
					{
					    ASSERT(!isForeignMachineAddr(pteToMachine(pte)));
					    paddr_t paddr = pteToPhysical(pte);

					    // logically free the frame
					    pageTablePfnUserFree(paddr>>FRAME_SHIFT);				
					}
				}

			    // free l1 pages
			    _ptpFree(pt, l2ptr);
			}
		    // free l2 pages
		    _ptpFree(pt, l3ptr);
		}
#ifdef HAS_L4
	    // free l3 pages
	    _ptpFree(pt, l4ptr);
	}
#endif
}

//______________________________________________________________________________
// Insert a level of the tree
//______________________________________________________________________________
static
vaddr_t
_insertPtp(ptentry_t  *pt,
	   ulong       offset,
	   maddr_t     maddr,
	   pteflags_t  pteFlags
	   )
{
    // need a page beneath this level
    ptentry_t *ptp = pageTablePtpAlloc();
    ASSERT(ptp);

    ptp[offset] = _pteCreate(maddr, pteFlags);

    // remove write permission from page table pages,
    // as required by Xen's page management
    ptentry_t *childPtePtr = archPteGet(pt, (vaddr_t) ptp);
    _ptpInternalUpdate(childPtePtr, *childPtePtr & ~_PAGE_RW);
    return (vaddr_t) ptp;
}


//______________________________________________________________________________
// Insert a level of the tree
//______________________________________________________________________________
static
ptentry_t *
_insertInternal(pt_t       pt,          ///< page table 
		vaddr_t    vaddr,       ///< the new virtual address
		maddr_t    maddr,
		pteflags_t leafFlags
		)
{
    ptentry_t *tab      = pt;
    vaddr_t    ptp      = 0;
    ptentry_t *ptePtr   = NULL;
    ptentry_t  pte;

#ifdef HAS_L4
    // X86-64
    ptePtr = tab + l4offset(vaddr);
    if (archPteIsFree(*ptePtr))
	{ 
	    ptp = _insertPtp(pt, l1offset(vaddr), maddr,                      leafFlags);
	    ptp = _insertPtp(pt, l2offset(vaddr), virtualToMachine(ptp),      L2_PROT);
	    ptp = _insertPtp(pt, l3offset(vaddr), virtualToMachine(ptp),      L3_PROT);
	    pte = _pteCreate(virtualToMachine(ptp), L4_PROT);
	    _ptpInternalUpdate(ptePtr, pte);
	    _cloneUser(USER_BASEPTR(ptePtr), pte);
	    return ptePtr;
	} 
    tab =  (ptentry_t *) pteToVirtual(*ptePtr);

#endif

    ptePtr = tab + l3offset(vaddr);
    if (archPteIsFree(*ptePtr))
	{   
	    ptp = _insertPtp(pt, l1offset(vaddr), maddr, leafFlags);
	    ptp = _insertPtp(pt, l2offset(vaddr), virtualToMachine(ptp),      L2_PROT);
	    pte = _pteCreate(virtualToMachine(ptp), L3_PROT);
	    _ptpInternalUpdate(ptePtr, pte);
	    return ptePtr;
	} 
    tab =  (ptentry_t *) pteToVirtual(*ptePtr);

    ptePtr = tab + l2offset(vaddr);
    if (archPteIsFree(*ptePtr))
	{   
	    ptp = _insertPtp(pt, l1offset(vaddr), maddr, leafFlags);
	    pte = _pteCreate(virtualToMachine(ptp), L2_PROT);
	    _ptpInternalUpdate(ptePtr, pte);
	    return ptePtr;
	} 
    tab =  (ptentry_t *) pteToVirtual(*ptePtr);


    ptePtr = tab + l1offset(vaddr);
    if (archPteIsFree(*ptePtr))
	{   
	    pte = _pteCreate(maddr, leafFlags);
	    _ptpInternalUpdate(ptePtr, pte);
	    return ptePtr;
	} 

    return NULL;
}



//______________________________________________________________________________
///  Insert a new page into the pt at the specified virtual address
///  and with the given permissions
//
// i7 clean
//______________________________________________________________________________
ptentry_t *
archPageTableInsert(pt_t    pt,                    ///< page table 
		    vaddr_t vaddr,                 ///< the new virtual address
		    maddr_t maddr,                 ///< the current machine address of the page to be mapped in
		    permission_t permission        ///< permissions
		    )
{

    pteflags_t pteflags = L1_PROT;
    if (isUserSpaceAddress(vaddr))
	{
	    pteflags = _pteFlags(permission, true);
	}

    ptentry_t *ptePtr =  _insertInternal(pt, vaddr, maddr, pteflags);
    ASSERT(ptePtr);

    _doUpdate();

    //_ptpI7(vaddr);

    return ptePtr;
}


//______________________________________________________________________________
// do a batch update of leaves, previous levels exist
//
// unused
//______________________________________________________________________________
void
archPageTablePageUpdate(pt_t         pt,
			pfn_t       *pfnArray,
			vaddr_t     *vaddrArray,
			ulong        count,
			permission_t permission)
{
    if (!count)
	{
	    return;
	}

    while (count--)
	{
	    vaddr_t    vaddr  = vaddrArray[count];
	    pfn_t      pfn    = pfnArray[count];
	    maddr_t    maddr  = pfnToMaddress(pfn);
	    pteflags_t pteFlags = _pteFlags(permission, true);
	    ptentry_t *ptePtr = _insertInternal(pt, vaddr, maddr, pteFlags);
    
	    // the l1 level entries
	    if (archPteIsFree(*ptePtr)) 
		{ 
		    ptentry_t pte = _pteCreate(maddr, pteFlags);
		    _ptpInternalUpdate(ptePtr, pte);
		}
	    else
		{
		    BUG();
		}
	}

    _doUpdate();
}

//______________________________________________________________________________
/// Gets the next valid address in the page table within [addr,hi)
//______________________________________________________________________________
vaddr_t
archPageTableNext(pt_t pt, vaddr_t addr, vaddr_t hi)
{
    ASSERT(pt);

    vaddr_t base[4] = {0, 0, 0, 0};
    ptentry_t *l4ptr, *l3ptr, *l2ptr, *l1ptr;

    if ((hi - addr) < PAGE_SIZE)
	{
	    return hi;
	}

    addr += PAGE_SIZE;

    vaddr_t offset1 = l1offset(addr);
    vaddr_t offset2 = l2offset(addr);
  
    ptentry_t pte = _pteCreate(virtualToMachine((vaddr_t) pt), PERM_ALL);
    l4ptr = l3ptr = &pte;

    vaddr_t offset3 = l3offset(addr);

#ifdef HAS_L4
    vaddr_t offset4 = l4offset(addr);//, maxl4 = l4offset(hi);

    ulong l4;
    for (l4=offset4; l4<L4_PAGETABLE_ENTRIES; offset3 = offset2 = offset1 = 0, l4++)
	{
	    l4ptr = pt + l4;
	    base[3] = l4 << L4_PAGETABLE_SHIFT;
	    if (base[3] >= hi)
		{
		    return hi;
		}
	    if (archPteIsFree(*l4ptr))
		{
		    continue;
		}
#endif

	    ptentry_t *t3 = (ptentry_t *) pteToVirtual(*l4ptr);

	    ulong l3;
	    for (l3=offset3; l3<L3_PAGETABLE_ENTRIES; offset2 = offset1 = 0, l3++)
		{
		    l3ptr = t3 + l3;
		    base[2] = base[3] + (l3 << L3_PAGETABLE_SHIFT);
		    if (base[2] >= hi)
			{
			    return hi;
			}
		    if (archPteIsFree(*l3ptr)) 
			{
			    continue;
			}

		    ptentry_t *t2 = (ptentry_t *) pteToVirtual(*l3ptr);
		    
		    ulong l2;
		    for (l2=offset2; l2<L2_PAGETABLE_ENTRIES; offset1 = 0, l2++) 
			{
			    l2ptr = t2 + l2;
			    base[1] = base[2] + (l2 << L2_PAGETABLE_SHIFT);
			    if (base[1] >= hi) 
				{
				    return hi;
				}
			    if (archPteIsFree(*l2ptr)) 
				{
				    continue;
				}

			    ptentry_t *t1 = (ptentry_t *) pteToVirtual(*l2ptr);

			    ulong l1;
			    for (l1=offset1; l1<L1_PAGETABLE_ENTRIES; l1++)
				{
				    l1ptr = t1 + l1;
				    base[0] = base[1] + (l1 << L1_PAGETABLE_SHIFT);
				    if (base[0] >= hi)
					{
					    return hi;
					}
				    if (!archPteIsFree(*l1ptr))
					{ // found it, compute address
					    return base[0];
					}
				}
			}
		}
#ifdef HAS_L4
	}
#endif

    return  hi;  // not found
}


//______________________________________________________________________________
/// copy page table pages from userspace address range [addr, hi) in pt to toPt
//______________________________________________________________________________
void
archPageTableUserspaceCopy(pt_t pt, vaddr_t addr, vaddr_t hi, pt_t toPt, vaddr_t offset, permission_t permission)
{
    ASSERT(pt);

    vaddr_t base[4] = {0, 0, 0, 0};
    ptentry_t *l4ptr, *l3ptr, *l2ptr, *l1ptr;

    if ((hi - addr) < PAGE_SIZE)
	{
	    return;
	}

    vaddr_t offset1 = l1offset(addr);
    vaddr_t offset2 = l2offset(addr);
  
    ptentry_t pte = _pteCreate(virtualToMachine((vaddr_t) pt), PERM_ALL);
    l4ptr = l3ptr = &pte;

    vaddr_t offset3 = l3offset(addr);

#ifdef HAS_L4
    vaddr_t offset4 = l4offset(addr);//, maxl4 = l4offset(hi);

    ulong l4;
    for (l4=offset4; l4<L4_PAGETABLE_ENTRIES; offset3 = offset2 = offset1 = 0, l4++)
	{
	    l4ptr = pt + l4;
	    base[3] = l4 << L4_PAGETABLE_SHIFT;
	    if (base[3] >= hi)
		{
		    return;
		}
	    if (archPteIsFree(*l4ptr))
		{
		    continue;
		}
#endif

	    ptentry_t *t3 = (ptentry_t *) pteToVirtual(*l4ptr);

	    ulong l3;
	    for (l3=offset3; l3<L3_PAGETABLE_ENTRIES; offset2 = offset1 = 0, l3++)
		{
		    l3ptr = t3 + l3;
		    base[2] = base[3] + (l3 << L3_PAGETABLE_SHIFT);
		    if (base[2] >= hi)
			{
			    return;
			}
		    if (archPteIsFree(*l3ptr)) 
			{
			    continue;
			}

		    ptentry_t *t2 = (ptentry_t *) pteToVirtual(*l3ptr);
		    
		    ulong l2;
		    for (l2=offset2; l2<L2_PAGETABLE_ENTRIES; offset1 = 0, l2++) 
			{
			    l2ptr = t2 + l2;
			    base[1] = base[2] + (l2 << L2_PAGETABLE_SHIFT);
			    if (base[1] >= hi) 
				{
				    return;
				}
			    if (archPteIsFree(*l2ptr)) 
				{
				    continue;
				}

			    ptentry_t *t1 = (ptentry_t *) pteToVirtual(*l2ptr);

			    ulong l1;
			    for (l1=offset1; l1<L1_PAGETABLE_ENTRIES; l1++)
				{
				    l1ptr = t1 + l1;
				    base[0] = base[1] + (l1 << L1_PAGETABLE_SHIFT);
				    if (base[0] >= hi)
					{
					    return;
					}
				    if (!archPteIsFree(*l1ptr))
					{   // found it, compute address
					    // Apply the rights.
					    vaddr_t fromAddr = base[0];
					    vaddr_t   toAddr = fromAddr + offset;

					    // If there is a page a fromAddr, ...
					    maddr_t maddr = pteToMachine(*l1ptr);
					    ASSERT(maddr);

					    PhysicalInfo *physicalInfo = physicalToPhysicalInfo(machineToPhysical(maddr));
					    ASSERT(!test_bit(PG_pt,  &physicalInfo->flags));
					    ASSERT(!test_bit(PG_ptp, &physicalInfo->flags));
					    ASSERT(physicalInfo->count);

					    physicalInfoPageAlloc(physicalInfo);

					    // Map local_address to page.
					    archPageTableOfflineUserInsert(toPt, toAddr, maddr, permission);
					}
				}
			}
		}
#ifdef HAS_L4
	}
#endif
}

//______________________________________________________________________________
/// set pte flags for an address range [addr,hi) in pt
//______________________________________________________________________________
void
_rangeSet(pt_t pt, vaddr_t addr, vaddr_t hi, pteflags_t pteFlags)
{
    ASSERT(pt);

    vaddr_t base[4] = {0, 0, 0, 0};
    ptentry_t *l4ptr, *l3ptr, *l2ptr, *l1ptr;

    if ((hi - addr) < PAGE_SIZE)
	{
	    return;
	}

    vaddr_t offset1 = l1offset(addr);
    vaddr_t offset2 = l2offset(addr);
  
    ptentry_t pte = _pteCreate(virtualToMachine((vaddr_t) pt), PERM_ALL);
    l4ptr = l3ptr = &pte;

    vaddr_t offset3 = l3offset(addr);

#ifdef HAS_L4
    vaddr_t offset4 = l4offset(addr);//, maxl4 = l4offset(hi);

    ulong l4;
    for (l4=offset4; l4<L4_PAGETABLE_ENTRIES; offset3 = offset2 = offset1 = 0, l4++)
	{
	    l4ptr = pt + l4;
	    base[3] = l4 << L4_PAGETABLE_SHIFT;
	    if (base[3] >= hi)
		{
		    return;
		}
	    if (archPteIsFree(*l4ptr))
		{
		    continue;
		}
#endif

	    ptentry_t *t3 = (ptentry_t *) pteToVirtual(*l4ptr);

	    ulong l3;
	    for (l3=offset3; l3<L3_PAGETABLE_ENTRIES; offset2 = offset1 = 0, l3++)
		{
		    l3ptr = t3 + l3;
		    base[2] = base[3] + (l3 << L3_PAGETABLE_SHIFT);
		    if (base[2] >= hi)
			{
			    return;
			}
		    if (archPteIsFree(*l3ptr)) 
			{
			    continue;
			}

		    ptentry_t *t2 = (ptentry_t *) pteToVirtual(*l3ptr);
		    
		    ulong l2;
		    for (l2=offset2; l2<L2_PAGETABLE_ENTRIES; offset1 = 0, l2++) 
			{
			    l2ptr = t2 + l2;
			    base[1] = base[2] + (l2 << L2_PAGETABLE_SHIFT);
			    if (base[1] >= hi) 
				{
				    return;
				}
			    if (archPteIsFree(*l2ptr)) 
				{
				    continue;
				}

			    ptentry_t *t1 = (ptentry_t *) pteToVirtual(*l2ptr);

			    ulong l1;
			    for (l1=offset1; l1<L1_PAGETABLE_ENTRIES; l1++)
				{
				    l1ptr = t1 + l1;
				    base[0] = base[1] + (l1 << L1_PAGETABLE_SHIFT);
				    if (base[0] >= hi)
					{
					    return;
					}
				    if (!archPteIsFree(*l1ptr))
					{ // found it, compute address
					    // Apply the rights.
					    ptentry_t *ptePtr = l1ptr;
					    ptentry_t newPte = _pteCreate(*ptePtr, pteFlags);

					    //xprintLog("archPageTableProtectRange:  addr =  $[xlong]    old = $[xint64]   new = $[xint64]\n",
					    //   addr, *ptePtr, newPte);

					    _ptpInternalUpdate(ptePtr, newPte);

					    _ptpI7(addr);
					}
				}
			}
		}
#ifdef HAS_L4
	}
#endif
}

//______________________________________________________________________________
// Walk PTs for vaddr
//______________________________________________________________________________
void
archPageTableWalk(vaddr_t vaddr)
{
    pt_t table = currentPt; // current page table
    ptentry_t pte;  // current page table entry


    xprintLog("Walking address $[pointer]"PRIpte"\n", vaddr);

    xprintLog("CR3: $[pointer]\n", (ulong)table);

#ifdef HAS_L4
    pte = table[l4offset(vaddr)];
    if ((pte & _PAGE_PRESENT) == 0) 
	{
	    printfLog("L4: 0x%"PRIpte" [0x%x] (page not present)\n", pte, l4offset(vaddr));
	    return;
	}

    table = (ptentry_t *)pteToVirtual(pte);
    printfLog("L4: 0x%"PRIpte" (0x%x) [0x%x]\n", pte, (ulong)table,
	   l4offset(vaddr));
#endif

    pte = table[l3offset(vaddr)];
    if ((pte & _PAGE_PRESENT) == 0) 
	{
	    printfLog("L3: 0x%"PRIpte" [0x%x] (page not present)\n", pte,
		   l3offset(vaddr));
	    return;
	}
    table = (ptentry_t *)pteToVirtual(pte);
    printfLog("L3: 0x%"PRIpte" (0x%p) [0x%x]\n", pte, (ulong)table,
	   l3offset(vaddr));

    pte = table[l2offset(vaddr)];
    if ((pte & _PAGE_PRESENT) == 0) 
	{
	    printfLog("L2: 0x%"PRIpte" [0x%x] (page not present)\n", pte,
		   l2offset(vaddr));
	    return;
	}
    table = (ptentry_t *)pteToVirtual(pte);
    printfLog("L2: 0x%"PRIpte" (0x%p) [0x%x]\n", pte, (ulong)table,
	   l2offset(vaddr));

    pte = table[l1offset(vaddr)];
    if ((pte & _PAGE_PRESENT) == 0) 
	{
	    printfLog("L1: 0x%"PRIpte" [0x%x] (page not present)\n", pte,
		   l1offset(vaddr));
	    return;
	}
    table = (ptentry_t *)pteToVirtual(pte);
    printfLog("L1: 0x%"PRIpte" (0x%p) [0x%x]\n", pte, (ulong)table,
	   l1offset(vaddr));
}

//______________________________________________________________________________
// Walk PTs for a snapshot of process's user space physical page usage
//______________________________________________________________________________
void
archPageTableStatistics(pt_t pageTable)
{
    ASSERT(pageTable);

    pt_t	l2ptBase,		// page directory table base
    		l1ptBase;		// page table base

    ptentry_t	l3pte,		// page directory (pointer) entry
	        l2pte,		// page directory entry
	        l1pte;		// page table entry

    xprintLog("CR3: $[pointer]. ", (ulong)pageTable);

    ulong l3, l2, l1;
    ulong pageCount = 0;

    pageCount++;	// page directory pointer table

#ifdef HAS_L4
    ulong l4;
    pt_t l3ptBase;		// page directory table base
    ptentry_t l4pte;	// page directory (pointer) entry

    // FIXME: skip kernel address space in L4
    for (l4=0; l4<L4_USER_ENTRIES; l4++)
	{
	    l4pte = pageTable[(vaddr_t)l4];
	    if ((l4pte & _PAGE_PRESENT) == 0)
		{
		    continue;
		}
	    pageCount++;

	    l3ptBase = (ptentry_t *)pteToVirtual(l4pte);
	    for (l3=0; l3<L3_USER_ENTRIES; l3++)
		{
		    l3pte = l3ptBase[(vaddr_t)l3];
		    if ((l3pte & _PAGE_PRESENT) == 0)
			{
			    continue;
			}
		    pageCount++;
#else
	    for (l3=0; l3<L3_USER_ENTRIES; l3++)
		{
	    	l3pte = pageTable[(vaddr_t)l3];
		    if ((l3pte & _PAGE_PRESENT) == 0)
			{
			    continue;

			}
		    pageCount++;
#endif

		    l2ptBase = (ptentry_t *)pteToVirtual(l3pte);
		    for (l2=0; l2<L2_USER_ENTRIES; l2++)
			{
		    	l2pte = l2ptBase[(vaddr_t)l2];
			    if ((l2pte & _PAGE_PRESENT) == 0)
				{
				    continue;
				}
			    pageCount++;

			    l1ptBase = (ptentry_t *)pteToVirtual(l2pte);
			    for (l1=0; l1<L1_USER_ENTRIES; l1++)
				{
			    	l1pte = l1ptBase[(vaddr_t)l1];
				    if ((l1pte & _PAGE_PRESENT) == 0)
				    {
					    continue;
				    }
				    pageCount++;
				} // end l1
			} // end l2
#ifdef HAS_L4
		} // end l3
	} // end l4
#else
		} // end l3
#endif

    xprintLog("Total number of present userspace pages: $[ulong]\n", pageCount);
}

//______________________________________________________________________________
/// initial build of page tables from Xen allocation
//______________________________________________________________________________
void
archPageTablePopulate(pfn_t *startPfnPtr, pfn_t *maxMappedPfnPtr, pfn_t *maxPfnPtr)
{
    pfn_t pfn;
    ptentry_t *result;

    currentPt = (pt_t) start_info.pt_base;

    // First page follows page table pages. This where we will start placing down the page tables.
    // Yes, it appears that the frames mapped at VIRT_START are in order =).
    startPfn = PFN_UP(virtualToPhysical(start_info.pt_base)) + start_info.nr_pt_frames; 
  
    // Total RAM pages.
    pfn_t maxPfn = start_info.nr_pages;

    xprintLog("  _text:        $[pointer]\n", &_text);
    xprintLog("  _etext:       $[pointer]\n", &_etext);
    xprintLog("  _edata:       $[pointer]\n", &_edata);
    xprintLog("  stack start:  $[pointer]\n", stack);
    xprintLog("  _end:         $[pointer]\n", &_end);

    pfn_t initPfnToMap = (start_info.nr_pt_frames - NOT_L1_FRAMES) * L1_PAGETABLE_ENTRIES;
    xprintLog("  First free, but mapped, pfn:   $[pointer]\n", startPfn);
    xprintLog("  Fitst unmapped pfn:            $[pointer]\n", initPfnToMap);
    xprintLog("  Total RAM pages:               $[pointer]\n", maxPfn);
    xprintLog("  Total RAM pages used for PTs:  $[pointer]\n", start_info.nr_pt_frames);

    xprintLog("  KERN_START                     $[pointer]\n", KERN_START);
    xprintLog("  KERN_END                       $[pointer]\n", KERN_END);


    pfn_t maxMappedPfn = (KERN_END - KERN_START) >> PAGE_SHIFT;
    xprintLog("  Max number of mapped pfns:     $[xlong]\n", maxMappedPfn);
    if (maxPfn < maxMappedPfn)
	{  // too much memory, lets limit it to the kernel address space
	    maxMappedPfn = maxPfn;
	}

    printfLog("  Mappable pages: 0x%lx-0x%lx    unmapped pages: 0x%lx-0x%lx\n", startPfn, maxMappedPfn, maxMappedPfn, maxPfn);


    // We worked out the virtual memory range to map, now mapping loop
    printfLog("Mapping memory range 0x%lx - 0x%lx\n", pfnToVirtual(initPfnToMap), pfnToVirtual(maxMappedPfn));

    int x=0;
    // these addresses are already mapped by Xen, check them
    for (pfn=startPfn; pfn<initPfnToMap; pfn++)
	{
	    x += *(int*) pfnToVirtual(pfn);  // de-reference and check that page fault is not a problem
	}

    printfLog("walked initially mapped pages\n");

    // now map the rest of the addresses
    for (pfn=initPfnToMap; pfn<maxMappedPfn; pfn++)
	{
	    vaddr_t vaddr = pfnToVirtual(pfn);  // determine where to map address

	    // make sure virtual address is in kernel address space
	    BUG_ON(vaddr < KERN_START || vaddr >= KERN_END);

	    maddr_t maddr = virtualToMachine(vaddr);

	    result = archPageTableInsert((pt_t) start_info.pt_base,
					 vaddr,
					 maddr,
					 0 /* ignored */
					 );
	    BUG_ON(NULL==result);

	    x += *(int*) pfnToVirtual(pfn);  // check page is mapped
	}

    printfLog("mapped rest of pages\n");

    // return the pfn range
    *startPfnPtr     = startPfn;       // first non-pte page
    *maxMappedPfnPtr = maxMappedPfn;   // last kernel mapped pfn
    *maxPfnPtr       = maxPfn;         // max page allocated to this domain
}

//______________________________________________________________________________
/// kernel top page table entry missing, propagate it from master kernel page table
//______________________________________________________________________________
void
archPageTablePropagateKernelEntry(pt_t    currentPt,
				  vaddr_t faultingAddress
				  )
{
    ptentry_t *source_pgde      = (ptentry_t *) start_info.pt_base;
    ptentry_t *destination_pgde = currentPt;
    vaddr_t    offset;

#if defined(__x86_32__)
    // 32-bit  (kernel levels below 2 are shared)

    offset           = l3offset(faultingAddress);
    source_pgde      = (ptentry_t *) pteToVirtual(source_pgde[offset]);
    destination_pgde = (ptentry_t *) pteToVirtual(destination_pgde[offset]);

    offset           = l2offset(faultingAddress);
    source_pgde      = (ptentry_t *) source_pgde      + offset;
    destination_pgde = (ptentry_t *) destination_pgde + offset;
#else
    // 64-bit  (kernel levels below 4 are shared)

    offset           = l4offset(faultingAddress);
    source_pgde      = source_pgde      + offset;
    destination_pgde = destination_pgde + offset;
#endif
    
    _ptpInternalUpdate(destination_pgde, *source_pgde);
    _doUpdate();
}

//______________________________________________________________________________
/// removes a page 
//
// i7: need to also update page mapping
//______________________________________________________________________________
void
archPageTableRemove(
		    pt_t     pt,
		    vaddr_t  vaddr
		    )
{
    ASSERT(pt);

    ptentry_t *ptePtr = archPteGet(pt, vaddr);
    ASSERT(ptePtr);

    _ptpInternalUpdate(ptePtr, 0); // zero out page entry
    _doUpdate();

    // i7
    _ptpI7(vaddr);
}

//______________________________________________________________________________
// replace a page table entry with another
//______________________________________________________________________________
void
archPageTableReplace(pt_t         pt,
		     vaddr_t      vaddr,
		     maddr_t      maddr,
		     permission_t permission
		     )
{
    ASSERT(pt);

    ptentry_t *ptePtr = archPteGet(pt, vaddr);
    ASSERT(ptePtr);
    ptentry_t  pte    = _pteCreate(maddr, _pteFlags(permission, true));
    //xprintLog("$[str]: $[xint64]\n", __func__, pte);

    _ptpInternalUpdate(ptePtr, pte); // replace page entry

    _doUpdate();

    // i7
    _ptpI7(vaddr);

}

//______________________________________________________________________________
/// returns paddr if not foreign, 0 if foreign
//
//  read only
//______________________________________________________________________________
paddr_t
archPageTableToPaddr(pt_t pt, vaddr_t addr)
{
    ASSERT(pt);

    ptentry_t *ptePtr = archPteGet(pt, addr);
  
    ASSERT(ptePtr);

    if (isForeignMachineAddr(pteToMachine(*ptePtr)))
	{
	    return (paddr_t) 0;
	}
    else 
	{
	    return pteToPhysical(*ptePtr);
	}

}

//______________________________________________________________________________
// print out user-space entries of page table
//______________________________________________________________________________
static
void
_printUserspace1(ptentry_t pte)
{

    ptentry_t *ptp = (ptentry_t *) pteToVirtual(pte);
    ASSERT(ptp);

    ulong l;
    for (l=0; l<L1_PAGETABLE_ENTRIES; l++)
	{
	    ptentry_t pte = ptp[l];
	    if (archPteIsFree(pte))
		{
		    continue;
		}
	    xprintLog("$[str]: level1 $[xint64]\n", __func__, pte);
	}
}

//______________________________________________________________________________
// print out user-space entries of page table
//______________________________________________________________________________
static
void
_printUserspace2(ptentry_t pte)
{

    ptentry_t *ptp = (ptentry_t *) pteToVirtual(pte);
    ASSERT(ptp);

    ulong l;
    for (l=0; l<L2_PAGETABLE_ENTRIES; l++)
	{
	    ptentry_t pte = ptp[l];
	    if (archPteIsFree(pte))
		{
		    continue;
		}
	    xprintLog("$[str]: level2 $[xint64]\n", __func__, pte);

	    _printUserspace1(pte);
	}
}

#if defined(__x86_64__)
//______________________________________________________________________________
// print out user-space entries of page table
//______________________________________________________________________________
static
void
_printUserspace3(ptentry_t pte)
{

    ptentry_t *ptp = (ptentry_t *) pteToVirtual(pte);
    ASSERT(ptp);

    ulong l;
    for (l=0; l<L3_PAGETABLE_ENTRIES; l++)
	{
	    ptentry_t pte = ptp[l];
	    if (archPteIsFree(pte))
		{
		    continue;
		}
	    xprintLog("$[str]: level3 $[xint64]\n", __func__, pte);

	    _printUserspace2(pte);
	}
}
#endif

//______________________________________________________________________________
///  print out the user-space entries of a page table
//
//   Read-only
//______________________________________________________________________________
void
archPageTablePrintUserspace(pt_t pt)
{
    ulong l;
    for (l=0; l<PT_USER_ENTRIES; l++)
	{
	    ptentry_t pte = pt[l];
	    if (archPteIsFree(pte))
		{
		    continue;
		}

#if defined(__x86_32__)
	    xprintLog("$[str]: level3 $[xint64]\n", __func__, pte);

	    _printUserspace2(pte);
#else
	    xprintLog("$[str]: level4 $[xint64]\n", __func__, pte);

	    _printUserspace3(pte);
#endif
	}
}
