//_______________________________________________________________________________
/// Performs page allocation.  Every page is mapped into the kernel (even when
/// used in userspace).  Hence, page allocation allocates simultaneously an
/// unused kernel virtual address and physical address using a buddy allocator.
/// If the page is to be used in user space, it should also be mapped into
/// a userspace virtual address using VMAs.
//
// (C) 2003 - Rolf Neugebauer - Intel Research Cambridge
// Copyright (c) 2005, Keir A Fraser
// Updates: Andrei Warkentin (awarke2@uic.edu) for EthOS.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//_______________________________________________________________________________

#ifndef __MM_H__
#define __MM_H__

#include <nano/memory.h>

#if defined(__x86_32__)
#include <xen/arch-x86_32.h>
#elif defined(__x86_64__)
#include <xen/arch-x86_64.h>
#endif

#include <nano/core.h>
#include <nano/memory.h>

void    memoryInit(void);
void    pageInit(void);
vaddr_t pageKernelAlloc(uint order);
void    pageKernelFree(void *pointer, uint order);
Status  pageTableKernelMap(ulong frameCount, mfn_t *frames, vaddr_t *arrayPtr);


//______________________________________________________________________________
/// check that address is in the interval [from, to)
//______________________________________________________________________________
static inline
bool
addressInRange(vaddr_t from,       ///< start of range
	       vaddr_t to,         ///< end of range
	       vaddr_t addr        ///< address in question
	       )
{
    return (addr >= from) && (addr < to);
}

//______________________________________________________________________________
/// check that addresses addr and addr+length is in the interval [from, to]
/// NB ensures there is no overflow, underflow; length==0 means the entire
/// address space.
//______________________________________________________________________________
static inline
bool
addressRangeInRange(vaddr_t from,    ///< start of range
		    vaddr_t to,      ///< end of range
		    vaddr_t addr,    ///< first address in question
		    vaddr_t length   ///< size in question
		    )
{
    return addressInRange(from, to, addr) && (length <= (to-addr));
}

//______________________________________________________________________________
/// check for overlaps
//______________________________________________________________________________
static inline
bool
addressRangeOverlap(vaddr_t from,    ///< start of range
		    vaddr_t to,      ///< end of range
		    vaddr_t addr,    ///< first address in question
		    vaddr_t length   ///< size in question
		    )
{
    // addr+length >= from
    return (addr < to) && ((addr>=from) || (length >= from-addr));
}

static inline
bool
addressInUserspace(vaddr_t vaddr)
{
    return addressInRange(USERSPACE_START, USERSPACE_END, vaddr);
}

static inline
bool
addressRangeInUserspace(vaddr_t vaddr, ulong length)
{
    return  addressRangeInRange(USERSPACE_START, USERSPACE_END, vaddr, length);
}

//______________________________________________________________________________
/// Allocates a single page
//______________________________________________________________________________
static inline
vaddr_t
pageKernelAllocSingle(void)
{
	return pageKernelAlloc(0);
}

//______________________________________________________________________________
/// Free a single page
//______________________________________________________________________________
static inline
void
pageKernelFreeSingle(void *ptr)
{
	pageKernelFree(ptr,0);
}

//______________________________________________________________________________
/// get_order, from size it determines ceil(log(size/PAGE_SIZE)).
//______________________________________________________________________________
static inline
int
get_order(
	  vaddr_t size       ///< size in bytes
	  )
{
    vaddr_t order;
    vaddr_t pages = (size-1) >> PAGE_SHIFT;
    for ( order = 0; pages; order++ )
        pages >>= 1;

    return order;
}

#endif /* __MM_H__ */
