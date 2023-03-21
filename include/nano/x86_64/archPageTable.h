//______________________________________________________________________________
/// this is the information which has to do with multi-level page tables,
/// and are intended to be used only in very limited parts of the kernel.
//______________________________________________________________________________

/* -*-  Mode:C; c-basic-offset:4; tab-width:4 -*-
 *
 * (C) 2003 - Rolf Neugebauer - Intel Research Cambridge
 * Copyright (c) 2005, Keir A Fraser
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */


#ifndef x86_64_PAGE_TABLE_H_
#define x86_64_PAGE_TABLE_H_

#include <nano/x86_64/memory.h>
#include <xen/xen.h>
#include <xen/arch-x86_64.h>
#include <nano/ethosTypes.h>

#define L1_FRAME                1
#define L2_FRAME                2
#define L3_FRAME                3

#define HAS_L4

#define L2_PAGETABLE_SHIFT      21
#define L3_PAGETABLE_SHIFT      30
#define L4_PAGETABLE_SHIFT      39

#define L1_PAGETABLE_ENTRIES    512
#define L2_PAGETABLE_ENTRIES    512
#define L3_PAGETABLE_ENTRIES    512
#define L4_PAGETABLE_ENTRIES    512

#define L1_USER_ENTRIES         L1_PAGETABLE_ENTRIES
#define L2_USER_ENTRIES         L2_PAGETABLE_ENTRIES
#define L3_USER_ENTRIES         L3_PAGETABLE_ENTRIES
#define L4_USER_ENTRIES         ((paddr_t)USERSPACE_END >> L4_PAGETABLE_SHIFT)  
#define PT_USER_ENTRIES         L4_USER_ENTRIES
#define PT_KERNEL_ENTRIES       (L4_PAGETABLE_ENTRIES - PT_USER_ENTRIES)

#define PT_PAGE_ORDER           1

// These are page-table limitations. Current CPUs support only 40-bit phys.
#define PADDR_BITS              52
#define VADDR_BITS              48
#define PADDR_MASK              ((1UL << PADDR_BITS)-1)
#define VADDR_MASK              ((1UL << VADDR_BITS)-1)

#define L1_MASK  ((1UL << L2_PAGETABLE_SHIFT) - 1)
#define L2_MASK  ((1UL << L3_PAGETABLE_SHIFT) - 1)
#define L3_MASK  ((1UL << L4_PAGETABLE_SHIFT) - 1)

#define NOT_L1_FRAMES           3
#define PRIpte "016lx"
#define PAGETABLE_LEVELS        2

#define XEN_PAGETABLE_PIN_COMMAND       MMUEXT_PIN_L4_TABLE
#define CR3_TO_VIRT(x)                  (pteToVirtual((ptentry_t)(x)))
#define PADDR_PTE_MASK                  (PADDR_MASK & FRAME_MASK)
#define __pte(x)                        ((pte_t) { (x) } )

// level 4 page table offset from virtual address
static inline
vaddr_t
l4offset(vaddr_t vaddr)
{
    return (vaddr >> L4_PAGETABLE_SHIFT) & (L4_PAGETABLE_ENTRIES - 1);
}

// level 1 (leaf) page table offset from virtual address
static inline
vaddr_t
l1offset(vaddr_t vaddr)
{
    return (vaddr >> L1_PAGETABLE_SHIFT) & (L1_PAGETABLE_ENTRIES - 1);
}

// level 2 page table offset from virtual address
static inline
vaddr_t
l2offset(vaddr_t vaddr)
{
    return (vaddr >> L2_PAGETABLE_SHIFT) & (L2_PAGETABLE_ENTRIES - 1);
}

// level 3 page table offset from virtual address
static inline
vaddr_t
l3offset(vaddr_t vaddr)
{
    return (vaddr >> L3_PAGETABLE_SHIFT) & (L3_PAGETABLE_ENTRIES - 1);
}

// These are bits for page table entry
#define _PAGE_PRESENT  ((ptentry_t) 0x001)
#define _PAGE_RW       ((ptentry_t) 0x002)
#define _PAGE_USER     ((ptentry_t) 0x004)
#define _PAGE_PWT      ((ptentry_t) 0x008)
#define _PAGE_PCD      ((ptentry_t) 0x010)
#define _PAGE_ACCESSED ((ptentry_t) 0x020)
#define _PAGE_DIRTY    ((ptentry_t) 0x040)
#define _PAGE_PAT      ((ptentry_t) 0x080)
#define _PAGE_PSE      ((ptentry_t) 0x080)
#define _PAGE_GLOBAL   ((ptentry_t) 0x100)

// A pt entry can be present. Then you can do some other checks like 
// seeing if there were write rights. The problem is in what we want to
// represent ERM_NONE protections as. Just keeping the page "not present" 
// isn't good enough - we can't tell if this is an invalid entry or a PERM_NONE one.
// So we leverage the PSE bit, which doesn't mean anything without the present bit.
// It doesn't really matter which bit, as they all have no meaning without the present bit.
#define _PAGE_PERM_NONE  ((ptentry_t) 0x080)

// for some reason write permission is given to page
// table entries even though Xen forbids it.
//
// The permission on levels above L1 do not affect R,W,X access
// to pages, only the L1 pages do.  The access permission at L1
// are _PAGE_RW and _PAGE_USER.  Suspect that higher page table
// levels may summarize lower levels, thus giving page faults
// if permissions are not a superset of lower level permissions.
#define L1_PROT (_PAGE_PRESENT|_PAGE_RW|_PAGE_ACCESSED|_PAGE_USER)
#define L2_PROT (_PAGE_PRESENT|_PAGE_RW|_PAGE_ACCESSED|_PAGE_DIRTY|_PAGE_USER)
#define L3_PROT (_PAGE_PRESENT|_PAGE_RW|_PAGE_ACCESSED|_PAGE_DIRTY|_PAGE_USER)
#define L4_PROT (_PAGE_PRESENT|_PAGE_RW|_PAGE_ACCESSED|_PAGE_DIRTY|_PAGE_USER)

// Pagetable walking.
static inline
maddr_t
pteToMachine(ptentry_t pte)
{
    return pte & PADDR_PTE_MASK;
}

static inline
mfn_t
pteToMfn(ptentry_t pte)
{
    return pteToMachine(pte) >> L1_PAGETABLE_SHIFT;
}

static inline
vaddr_t
pteToVirtual(ptentry_t pte)
{
    return mfnToVirtual(pteToMfn(pte));
}

static inline
paddr_t
pteToPhysical(ptentry_t pte)
{
    return machineToPhysical(pte & PAGE_MASK);
}

#endif /* _ARCH_PAGE_H_ */
