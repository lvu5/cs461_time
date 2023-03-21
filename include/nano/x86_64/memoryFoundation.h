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

#ifndef __X86_64_MEMORYFOUNDATION_H__
#define __X86_64_MEMORYFOUNDATION_H__

#include <nano/ethosTypes.h>

//______________________________________________________________________________
// Definitions for machine and pseudo-physical addresses.
//     paddr_t:   physical address
//     pfn_t:     page frame number
//______________________________________________________________________________
typedef ulong paddr_t;
typedef ulong pfn_t;

#define L1_PAGETABLE_SHIFT 12

#define PAGE_SIZE       (1UL << L1_PAGETABLE_SHIFT)
#define FRAME_SIZE      PAGE_SIZE

#define PAGE_SHIFT          L1_PAGETABLE_SHIFT
#define PAGE_INTERIOR_MASK  (PAGE_SIZE-1)
#define PAGE_MASK           (~PAGE_INTERIOR_MASK)
#define FRAME_SHIFT         L1_PAGETABLE_SHIFT
#define FRAME_INTERIOR_MASK (FRAME_SIZE-1)
#define FRAME_MASK          (~FRAME_INTERIOR_MASK)

// Where kernel is linked.
static const vaddr_t VIRT_START      = (vaddr_t)(&_text);

// return ceil pfn for this paddr
static inline pfn_t
PFN_UP(paddr_t paddr)
{
	return (pfn_t) ((paddr + FRAME_SIZE - 1) >> FRAME_SHIFT);
}

// return floor pfn for this vaddr
static inline pfn_t
PFN_DOWN(paddr_t paddr)	
{
	return (pfn_t) (paddr >> FRAME_SHIFT);
}

// WARNING!! virtualToPhysical and stuff using it can ONLY be used on:
// a) Addresses of stuff within the actual kernel.
// b) Addreses to pages returned by alloc_pages(...) and anything that uses.
// c) Stuff like the Xen-created pages the domain is born with (like start info).
// DO NOT USE on user space addresses, addresses in the KMAPPING area, etc.
paddr_t virtualToPhysical(vaddr_t x);
vaddr_t physicalToVirtual(paddr_t x);


static inline pfn_t
virtualToPfn(const vaddr_t virt)
{
	return PFN_DOWN(virtualToPhysical(virt));
}

#endif /* __X86_64_MEMORYFOUNDATION_H__ */
