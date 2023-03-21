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

#ifndef _ARCH_MEMORY_H_
#define _ARCH_MEMORY_H_

#include <stdint.h>
#include <xen/xen.h>

#if defined(__x86_32__)
#include <nano/x86_32/memory.h>
#elif defined(__x86_64__)
#include <nano/x86_64/memory.h>
#else
#error Unsupported architecture.
#endif

typedef vaddr_t vfn_t;

// straight array to do a physical to machine mapping
extern mfn_t *pfnToMfnArray;

// return true if vaddr is in kernel space
static inline
bool
isKernelSpaceAddress(vaddr_t vaddr)
{
	return (vaddr >= KERN_START) && (vaddr < KERN_END);
}

// return true if vaddr is in kernel space
static inline
bool
isUserSpaceAddress(vaddr_t vaddr)
{
	return (vaddr >= USERSPACE_START) && (vaddr < USERSPACE_END);
}

// returns pfn corresponding to mfn
static inline
pfn_t
mfnToPfn(mfn_t mfn)
{
	return machine_to_phys_mapping[mfn];
}

static inline
vaddr_t
vfnToVirtual(vfn_t vfn)
{
	vaddr_t vaddr = vfn;
	vaddr <<= PAGE_SHIFT;
	return vaddr;
}

// returns mfn corresponding to pfn
static inline
mfn_t
pfnToMfn(pfn_t pfn)
{
	return pfnToMfnArray[pfn];
}

static inline maddr_t
mfnToMaddr(mfn_t mfn)
{
	maddr_t maddr = mfn;
	return maddr << FRAME_SHIFT;
}

static inline paddr_t
pfnToPaddr(pfn_t pfn)
{
	return ((paddr_t) pfn) << FRAME_SHIFT;
}

// if foreign, than the mfn maps to ~0 from Xen
// I could not find any documentation for this,
// but if it changes, we can keep a bit vector for
// foreign-ness  --jas
static inline bool
isForeignMachineAddr(maddr_t maddr)
{
	return (~(pfn_t) 0) == machine_to_phys_mapping[maddr >> PAGE_SHIFT];
}

static inline
maddr_t
pfnToMaddress(pfn_t pfn)
{
	return mfnToMaddr(pfnToMfn(pfn));
}

// returns an maddr corresponding to a paddr
static inline maddr_t
physicalToMachine(paddr_t paddr)
{
	mfn_t     mfn = pfnToMfn(paddr >> FRAME_SHIFT);
	maddr_t maddr = mfnToMaddr(mfn) | (paddr & FRAME_INTERIOR_MASK);
	return maddr;
}

// returns a paddr corresponding to a maddr
static inline paddr_t
machineToPhysical(maddr_t maddr)
{
	pfn_t   pfn   = mfnToPfn(maddr >> FRAME_SHIFT);
	paddr_t paddr = pfnToPaddr(pfn) | (maddr & FRAME_INTERIOR_MASK);
	return paddr;
}

static inline mfn_t
virtualToMfn(vaddr_t x)
{
	return pfnToMfn(virtualToPhysical(x) >> FRAME_SHIFT);
}

static inline maddr_t
virtualToMachine(vaddr_t virt)
{
	return physicalToMachine(virtualToPhysical(virt));
}

// This should be safe to use on most physical addresses,
// other than those that don't end up getting mapped because
// the total amount of physical space encroaches on the MAPPING area
// or beginning of Xen.
// Obviously you're getting bogus data for invalid MFNs, so Caveat Emptor!
static inline vaddr_t
machineToVirtual(maddr_t maddr)
{
	return physicalToVirtual(machineToPhysical(maddr));
}

static inline vaddr_t
mfnToVirtual(mfn_t mfn)
{
	return physicalToVirtual(pfnToPaddr(mfnToPfn(mfn)));
}

static inline vaddr_t
pfnToVirtual(pfn_t pfn)
{
	return physicalToVirtual(pfnToPaddr(pfn));
}



pfn_t maxMappedPfn;
pfn_t maxPfn;

#endif /* _ARCH_MEMORY_H_ */
