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

#ifndef __X86_64_MEMORY_H__
#define __X86_64_MEMORY_H__

#include <nano/ldsyms.h>
#include <nano/constant.h>
#include <nano/x86_64/memoryFoundation.h>
#include <nano/permission.h>

//______________________________________________________________________________
// Definitions for machine and pseudo-physical addresses.
//     maddr_t:   machine address
//     void*:     virtual address
//______________________________________________________________________________
typedef unsigned long maddr_t;
typedef unsigned long mfn_t;

typedef struct { unsigned long pte; } pte_t;

// To align the pointer to the (next) page boundary.
#define PAGE_ALIGN(addr)        (((addr) + PAGE_SIZE - 1) & PAGE_MASK)

// To align the pointer to the (next) frame boundary.
#define FRAME_ALIGN(addr)       (((addr) + FRAME_SIZE - 1) & FRAME_MASK)

//______________________________________________________________________________
//  Notation: [x, y) means the range from x...y-1
//
//  Here is how memory is laid out:
//
//  User memory    [0...1<<48) (see x86_64 ABI)
//  Hypervisor     [HYPERVISOR_VIRT_START... HYPERVISOR_VIRT_END)
//  Kmapping area  [KMAPPING_START...VIRT_START)
//  Kernel memory  [VIRT_START...)
//
//  Note that for Ethos managed memory, we assume that Ethos does not
//  use the last page of memory in the address space.  Given that the
//  hypervisor is mapped into high memory, this is primarily effects
//  x86-PAE memory layout.
//______________________________________________________________________________
// Program address space.
static const vaddr_t USERSPACE_START = 0;
static const vaddr_t USERSPACE_END   = 1UL<<47;

// Program initial stack.
#define USERSPACE_STACK_IN_PAGES   ((vaddr_t)(1<<10))
#define USERSPACE_STACK_SIZE       ((vaddr_t)USERSPACE_STACK_IN_PAGES * (vaddr_t)PAGE_SIZE)
#define USERSPACE_STACK_START      ((vaddr_t)USERSPACE_END - (vaddr_t)USERSPACE_STACK_SIZE)


// Where kernel is linked (see also memoryFoundation.h).
static const vaddr_t VIRT_END        = (vaddr_t)(&_etext);

static const vaddr_t KERN_START      = (vaddr_t)&_text;
static const vaddr_t KERN_END        = (vaddr_t)~0UL;

typedef paddr_t       ptentry_t;  // ptentry contains a physical addr (plus low order flags)
typedef ptentry_t    *pt_t;       // a page table is an array fo ptentry_t structurs
typedef unsigned long pteflags_t;

// true iff the page table entry is unused
static inline bool
archPteIsFree(ptentry_t pte)
{
  return 0 == pte;
}

// if in use, leaf multi-level tree node point to a
// valid machine address
static inline bool
archPteInUse(ptentry_t pte)
{
  return 0 != pte;
}


#endif /* __X86_64_MEMORY_H__ */
