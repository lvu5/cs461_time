//______________________________________________________________________________
// Ethos Hypervisor interface functions
// for the memory management subsystem.
//
// Satya Popuri <spopur2@uic.edu> 10/2007.
//
// These functions deal with privileged operations
// on page tables via Xen hypercalls. 
//
//______________________________________________________________________________

#include <nano/common.h>
#include <nano/mm.h>
#include <nano/physicalInfo.h>
#include <nano/pageTable.h>
#include <nano/xenEventHandler.h>
#include <nano/xenPageTable.h>
#include <nano/archPageTable.h>
#include <nano/fmt.h>

//______________________________________________________________________________
/// Pin a page descriptor.   Pinning tells Xen that this is (part of) a page table,
/// Xen then validates that the part of the page table reachable from the
/// pinned page is valid: i.e. that those pages are read only and that they
/// only point to machine pages held by the domain.
/// (Pinning makes context switching fast, since the page table is pre checked).
//______________________________________________________________________________
void
xenPageTablePin(pt_t pt)
{
    // pin the page directory.
    struct mmuext_op op;
    op.cmd = XEN_PAGETABLE_PIN_COMMAND;
    op.arg1.mfn = virtualToMfn((vaddr_t) pt);

    int reason  = HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF);
    if ( reason < 0) {
	printfLog("could not pin page table, error = %d\n", reason);
	BUG();
    }
    set_bit(PG_pinned, &virtualToPhysicalInfo(pt)->flags);
}


//______________________________________________________________________________
/// Unpin a page descriptor.  The (part of) the page table is no longer in use,
/// the pages can be released and recycled back into the domain's general pool.
//______________________________________________________________________________
void
xenPageTableUnpin(pt_t pt)
{
    // unpin
    struct mmuext_op op;
    op.cmd = MMUEXT_UNPIN_TABLE;
    op.arg1.mfn = virtualToMfn((vaddr_t) pt);
    BUG_ON(HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0);

    // re-obtain write access to pt after unipinning
    clear_bit(PG_pinned, &virtualToPhysicalInfo(pt)->flags);
}
