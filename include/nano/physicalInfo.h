//______________________________________________________________________________
// physical page information declarations.
//
//  1. Don't need to track kernel pages since these are never deleted
//  2. Don't need to track top-level page table (PG_pt) since that it destroyed
//     only when the process is destroyed
//  3. Track number of entries in non-top-level page table (PG_ptp) for ease of
//     garbage collection.
//  4. Track number of uses of a user page -- this also corresponds to the degree
//     of sharing.
//
// XXX: Probably Linux-tainted.
//
// (c) Jon A. Solworth  2009
//______________________________________________________________________________

#ifndef __ETHOS_PHYSICAL_INFO_H__
#define __ETHOS_PHYSICAL_INFO_H__

#include <ethos/list.h>


#define PG_other		 0	// unused
#define PG_pinned                1      // Xen-pinned page directory.
#define PG_pt                    2      // The root of a page table
#define PG_ptp                   3      // Page table pages (i.e., non-root, non-leaf pages).

/// Describes information for each physical page in the system.
/// Used for reference counting and typing (page table, etc.) underlying pages
typedef struct PhysicalInfo
{
    ulong flags;           ///< PG_* flags
    ulong count;           ///< count of number of places linked into page table beyond initial
                           ///< or if page table page the number of used entries
    vaddr_t vaddr;         ///< virtual address
} PhysicalInfo;

extern PhysicalInfo *physicalInfoArray;

#define ALIGN(x,a)              __ALIGN_MASK(x,(typeof(x))(a)-1)
#define __ALIGN_MASK(x,mask)    (((x)+(mask))&~(mask))


// Count equal 0 means there are no users.
static inline
bool
pageNotUsed(PhysicalInfo *x)
{
    ASSERT(x);
    return 0 == x->count;
}

// Frame is not mapped more than once.
// makes sense only if not part of the page tree
static inline int
pageShared(PhysicalInfo *x)
{
    ASSERT(x);
    ASSERT(!test_bit(PG_ptp, &x->flags));
    ASSERT(!test_bit(PG_pt, &x->flags));
    return x->count > 1;
}

// given a pointer in the kernel address space,
// returns the corresponding struct physicalInfo.
static inline PhysicalInfo*
virtualToPhysicalInfo(const  void *kaddr)
{
    PhysicalInfo *physicalInfo =  physicalInfoArray + (virtualToPhysical((vaddr_t)kaddr) >> PAGE_SHIFT);
    return physicalInfo;
}


static inline
PhysicalInfo*
pfnToPhysicalInfo(const pfn_t pfn)
{
    PhysicalInfo *physicalInfo = physicalInfoArray + pfn;
    return physicalInfo;
}

// given a pseudo-physical address in the kernel address space,
// returns the corresponding struct physicalInfo.
static inline
PhysicalInfo*
physicalToPhysicalInfo(const paddr_t paddr)
{
    PhysicalInfo *physicalInfo = pfnToPhysicalInfo(paddr >> PAGE_SHIFT);
    return physicalInfo;
}


// Update counter (used as PTE count)  on the page associated with the PT.
static inline void
physicalInfoPteAlloc(PhysicalInfo *physicalInfo)
{
    ASSERT(test_bit(PG_ptp, &physicalInfo->flags));
}

static inline void
physicalInfoPteFree(PhysicalInfo *physicalInfo)
{
    if (test_bit(PG_ptp, &physicalInfo->flags))
	{
	    ASSERT(!physicalInfo->count);
	} 
    else 
	{
	    ASSERT(test_bit(PG_pt, &physicalInfo->flags));
	}
}

static inline void
physicalInfoPageAlloc(PhysicalInfo *physicalInfo)
{
    physicalInfo->count++;
}

static inline void
physicalInfoPageFree(PhysicalInfo *physicalInfo)
{
    ASSERT(physicalInfo->count);
    physicalInfo->count--;
}


static inline
bool
physicalInfoUnusedPage(PhysicalInfo *physicalInfo)
{
    bool ok = !test_bit(PG_pt,  &physicalInfo->flags)
            & !test_bit(PG_ptp, &physicalInfo->flags)
            & !physicalInfo->count;
    return ok;
}

void  physicalInfoInit(void);
void  physicalInfoUnmap(pfn_t pfn);
void  physicalInfoMap(pfn_t pfn, vaddr_t vaddr);
pfn_t physicalAlloc(void);
bool  physicalFree(pfn_t pfn);

#endif // __ETHOS_PHYSICAL_INFO_H__
