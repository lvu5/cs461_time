//______________________________________________________________________________
// amd64-specific page table bits.
// Feb-2011: Pat Gavlin
//______________________________________________________________________________

#include <nano/common.h>
#include <nano/schedPrivileged.h>
#include <nano/pageTable.h>
#include <nano/archPageTable.h>

mfn_t pageTableMfn;
mfn_t pageTableUserMfn;

void
archPageTableLoad(void)
{
    if (!pageTableMfn)
	{
	    return;
	}

    struct mmuext_op op[2];

    op[0].cmd = MMUEXT_NEW_BASEPTR;
    op[0].arg1.mfn = pageTableMfn;

    // user page table should not map kernel pages since both run at ring 0
    op[1].cmd = MMUEXT_NEW_USER_BASEPTR;
    op[1].arg1.mfn = pageTableUserMfn;

    int res = HYPERVISOR_mmuext_op(op, 2, NULL, DOMID_SELF);
    if (res < 0)
	{
	    xprintLog("archPageTableSwitch: error $[int]\n", res);
	    BUG();
	}
}

void
archPageTableSwitch(pt_t pt)
{
    pageTableMfn     = virtualToMfn((vaddr_t) pt);
    pageTableUserMfn = virtualToMfn((vaddr_t) USER_BASEPTR(pt));
    archPageTableLoad();

    currentPt = pt;
}
