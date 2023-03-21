/******************************************************************************
 * setup.c
 * 
 * Common stuff special to x86 goes here.
 * 
 * Copyright (c) 2002-2003, K A Fraser & R Neugebauer
 * Copyright (c) 2005, Grzegorz Milos, Intel Research Cambridge
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
 *
 */

#include <nano/memory.h>
#include <nano/archPageTable.h>
#include <nano/common.h>
#include <nano/fmt.h>

// Shared page for communicating with the hypervisor.
// Events flags go here, for example.
shared_info_t *HYPERVISOR_shared_info;


// This structure contains start-of-day info, such as pagetable base pointer,
//address of the shared_info structure, and things like that.
union start_info_union start_info_union;

// Just allocate the kernel stack here. SS:ESP is set up to point here
// in head.S.
char stack[8192] __attribute__((aligned (16)));

extern char shared_info[PAGE_SIZE];

// Assembler interface fns in entry.S.
void failsafe_callback(void);


//______________________________________________________________________________
/// Map shared_info into page table 
//______________________________________________________________________________
static
shared_info_t *
_mapSharedInfo(paddr_t pa)
{
    if ( HYPERVISOR_update_va_mapping(
				      (ulong) shared_info,
				      __pte(pa | 7),
				      UVMF_INVLPG) )
	{
	    xprintLog("Failed to map shared_info!!\n");
	    BUG();
	}
    return (shared_info_t *) shared_info;
}

//______________________________________________________________________________
/// Copy over the start_info and other initialization
//______________________________________________________________________________
void
archInit(start_info_t *si)
{
    // Copy the start_info struct to a globally-accessible area.
    // WARN: don't do print before here, it uses information from
    // shared_info. Use xprint instead.
    memcpy(&start_info, si, sizeof(*si));

    // set up minimal memory infos 
    pfnToMfnArray = (ulong *) start_info.mfn_list;

    // Grab the shared_info pointer and put it in a safe place.
    HYPERVISOR_shared_info = _mapSharedInfo(start_info.shared_info);

    // Set up event and failsafe callback addresses. 
#ifdef __x86_32__
    HYPERVISOR_set_callbacks(
			     __KERNEL_CS, (unsigned long) &hypervisor_callback,
			     __KERNEL_CS, (unsigned long) failsafe_callback);
#elif defined(__x86_64__)
    HYPERVISOR_set_callbacks(
			     (ulong) &hypervisor_callback,
			     (ulong) &failsafe_callback, 0);
#endif
}

//______________________________________________________________________________
/// print stack location
//______________________________________________________________________________
void
archPrintInfo(void)
{
    printfLog("  stack:      %p-%p\n", stack, stack + sizeof(stack));
}
