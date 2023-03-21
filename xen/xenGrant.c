//___________________________________________________________________________
// (C) 2006 - Cambridge University
//___________________________________________________________________________
//
//        File: xenGrant.c
//      Author: Steven Smith (sos22@cam.ac.uk)
//     Changes: Grzegorz Milos (gm281@cam.ac.uk)
//        Date: July 2006
//
// Environment: Xen Minimal OS (adapted to Ethos)
/// Description: Grants other domain's access to Ethos pages (and vice versa).
///  Ethos page grants are made available to Xen by putting them in xenGrantTable,
///  which is kept in memory readable by Xen.
//
//  Simple grant table implementation. About as stupid as it's
//  possible to be and still work.
//
//   When updating xenGrantTable[i], the flags field must be updated
//   last (if it is zero the other fields are ignored) and a write barrier
//   (wmb()) must be done before the flags are updated.
//___________________________________________________________________________

#include <nano/common.h>
#include <nano/mm.h>
#include <nano/pageTable.h>
#include <nano/xenGrant.h>
#include <nano/xenEventHandler.h>

#define NR_RESERVED_ENTRIES 8
#define NR_GRANT_ENTRIES (NR_GRANT_FRAMES * PAGE_SIZE / sizeof(grant_entry_v1_t))

// this is going to contain the grant table entries which are mapped into both
// ethos and Xen.  Modification to these entries, therefore, cause Xen to
// perform grant actions.
static grant_entry_v1_t *xenGrantTable;

// list of available pages to grant, xenGrantList[0] 
// points to the first element in the list.
static grant_ref_t    xenGrantList[NR_GRANT_ENTRIES];

//______________________________________________________________________________
/// Free grant entry
//______________________________________________________________________________
static void
_putFreeEntry(grant_ref_t ref)
{
    xenGrantList[ref] = xenGrantList[0];
    xenGrantList[0]  = ref;
}

//______________________________________________________________________________
/// Find unused grant entry
//    should check list empty --jas
//______________________________________________________________________________
static grant_ref_t
_getFreeEntry(void)
{
    unsigned int ref = xenGrantList[0];
    xenGrantList[0] = xenGrantList[ref];
    return ref;
}


//______________________________________________________________________________
/// Allow shared access between domain.
//______________________________________________________________________________
grant_ref_t
xenGrantAccess(domid_t domid,          ///< domain to be granted access
	       mfn_t frame,            ///< page frame to be shared
	       int readonly            ///< true if read only
	       )
{
    grant_ref_t ref = _getFreeEntry();
    xenGrantTable[ref].frame = frame;
    xenGrantTable[ref].domid = domid;
    wmb();
    readonly *= GTF_readonly;
    xenGrantTable[ref].flags = GTF_permit_access | readonly;

    return ref;
}

//______________________________________________________________________________
/// Transfer page to another domain.
//______________________________________________________________________________
grant_ref_t
xenGrantTransfer(domid_t domid,       ///< domain to be transfer to
		 pfn_t pfn            ///< page frame number
		 )
{
    grant_ref_t ref = _getFreeEntry();
    xenGrantTable[ref].frame = pfn;
    xenGrantTable[ref].domid = domid;
    wmb();
    xenGrantTable[ref].flags = GTF_accept_transfer;

    return ref;
}

//______________________________________________________________________________
/// Remove page access from another domain
//______________________________________________________________________________
int
xenGrantEndAccess(grant_ref_t ref     ///< the grant entry of the page to be removed
		  )
{
    u16 flags, nflags;

    nflags = xenGrantTable[ref].flags;
    do {
	if ((flags = nflags) & (GTF_reading|GTF_writing)) {
	    printfLog("WARNING: attempted to end access to grant entry still in use!\n");
	    return 0;
	}
    } while ((nflags = synch_cmpxchg(&xenGrantTable[ref].flags, flags, 0)) !=
	     flags);

    _putFreeEntry(ref);
    return 1;
}

//______________________________________________________________________________
/// get transfered page back from domain
//______________________________________________________________________________
unsigned long
xenGrantEndTransfer(grant_ref_t ref    ///< the tranfered page grant table reference
		    )
{
    unsigned long frame;
    u16 flags;

    while (!((flags = xenGrantTable[ref].flags) & GTF_transfer_committed)) {
	if (synch_cmpxchg(&xenGrantTable[ref].flags, flags, 0) == flags) {
	    printfLog("Release unused transfer grant.\n");
	    _putFreeEntry(ref);
	    return 0;
	}
    }

    // If a transfer is in progress then wait until it is completed.
    while (!(flags & GTF_transfer_completed)) {
	flags = xenGrantTable[ref].flags;
    }

    // Read the frame number /after/ reading completion status.
    rmb();
    frame = xenGrantTable[ref].frame;

    _putFreeEntry(ref);

    return frame;
}

//______________________________________________________________________________
/// allocate a page of virtual memory and then grant to domain 0.
//______________________________________________________________________________
grant_ref_t
xenGrantAllocAndGrant(void **map        ///< returns the page allocated
		      )
{
    mfn_t mfn;
    grant_ref_t ref;

    *map = (void *) pageKernelAllocSingle();
    mfn = virtualToMfn((vaddr_t)*map);
    ref = xenGrantAccess(0, mfn, 0);
    return ref;
}

static const char * const xenGrantOpErrorMessages[] = GNTTABOP_error_msgs;

//______________________________________________________________________________
/// returns an error string corresponding to the grant table status
//______________________________________________________________________________
const char *
xenGrantOpError(int16_t status            ///< status return
		)
{
    status = -status;
    if (status < 0 || status >= ARRAY_SIZE(xenGrantOpErrorMessages))
	return "bad status";
    else
	return xenGrantOpErrorMessages[status];
}

//______________________________________________________________________________
/// Maps a grant ref from a foreign domain.
//______________________________________________________________________________
int
xenGrantMapForeignGrant(vaddr_t hostAddr,       ///< address to be mapped into
		uint32_t domId,                         ///< foreign domain id
		grant_ref_t ref,                        ///< foreign grant ref
		int readOnly,                           ///< true if read only
		grant_handle_t* handle                  ///< grant handle needed for unmap
		)
{
    gnttab_map_grant_ref_t op;
    int err;

    op.host_addr = hostAddr;
    op.ref = ref;
    op.dom = domId;
    op.flags = GNTMAP_host_map;
    if (readOnly)
	{
	    op.flags |= GNTMAP_readonly;
	}

    err = HYPERVISOR_grant_table_op(GNTTABOP_map_grant_ref, &op, 1);
    if (err != 0 || op.status != 0 )
	{
	    printfLog("xenGrantMapForeignGrant failed! err = %d, op.status = %d, %s\n",
			err, op.status, xenGrantOpError(op.status));
	    return err != 0 ? err : op.status;
	}
    *handle = op.handle;

    return 0;
}

//______________________________________________________________________________
/// Unmaps a grant ref from a foreign domain.
//______________________________________________________________________________
int
xenGrantUnmapForeignGrant(vaddr_t hostAddr,		///< address to be unmapped
		grant_handle_t handle		///< handle used for unmap
		)
{
    struct gnttab_unmap_grant_ref op;
    int err;

    op.host_addr = hostAddr;
    op.dev_bus_addr = 0;
    op.handle = handle;

    err = HYPERVISOR_grant_table_op(GNTTABOP_unmap_grant_ref, &op, 1);
    if (err != 0 || op.status != 0 )
	{
	    printfLog("xenGrantUnMapForeignGrant failed! err = %d, op.status = %d, %s\n",
			err, op.status, xenGrantOpError(op.status));
	    return err != 0 ? err : op.status;
	}

    return 0;
}
//______________________________________________________________________________
/// Initialize the grant table
//______________________________________________________________________________
void
xenGrantInit(void)
{
    int i;
    gnttab_setup_table_t setup;
    mfn_t frames[NR_GRANT_FRAMES];

    // initialize the free list
    for (i = NR_RESERVED_ENTRIES; i < NR_GRANT_ENTRIES; i++)
	{
	    _putFreeEntry(i);
	}

    setup.dom = DOMID_SELF;
    setup.nr_frames = NR_GRANT_FRAMES;
    // makes setup.frame_list point to frames
    set_xen_guest_handle(setup.frame_list, frames);

    // get the machine addresses in frames for the grant table, Xen will
    // watch these frames.
    // Allocation is in frames rather than grant_table_entries.
    HYPERVISOR_grant_table_op(GNTTABOP_setup_table, &setup, 1);

    printfLog("XEN_GRANT_ frames:\n");
    for (i = 0; i < NR_GRANT_FRAMES; i++)
	{
	    //ASSERT(isForeignMachineAddr(frames_addr[i])); // Does not work on PAE
	    printfLog("\tMFN = 0x%x\n", frames[i]);
	}

    // the frames returned from GNTTABOP_setup_table are mapped into virtual memory
    // and returned as xenGrantTable.
    Status status = pageTableKernelMap(NR_GRANT_FRAMES, frames, (vaddr_t *) &xenGrantTable);
    BUG_ON(StatusOk!=status);

    printfLog("xenGrantTable mapped at %p.\n", xenGrantTable);
    printfLog("NR_GRANT_ENTRIES: %d\n", NR_GRANT_ENTRIES);
}
