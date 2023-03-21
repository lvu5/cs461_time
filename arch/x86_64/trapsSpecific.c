#include <nano/common.h>
#include <nano/cpuPrivileged.h>
#include <nano/mm.h>

//______________________________________________________________________________
/// Returns the cpu priviledge state implied by regs in state.
//______________________________________________________________________________
ArchPrivilegeState
archPrivilegeState(arch_interrupt_regs_t *regs     ///< copy of the registers
		)
{
    ASSERT(regs);
    if ((regs->cs & SEGMENT_MASK) == (__KERNEL_CS & 0xfff0))
	{
	    return PRIVILEGED_MODE;
	}
    else if (unlikely((regs->cs & SEGMENT_MASK) != FLAT_USER_CS))
	{
	    BUG();
	}
    return NON_PRIVILEGED_MODE;
}


