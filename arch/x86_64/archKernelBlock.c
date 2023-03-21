//______________________________________________________________________________
/// Low-level process management bits.
// Mar-2008: Andrei Warkentin
// May-2010: Jon A. Solworth
//______________________________________________________________________________

#include <nano/common.h>
#include <nano/xenEvent.h>
#include <nano/mm.h>
#include <nano/timer.h>
#include <nano/xenSchedule.h>
#include <nano/archPageTable.h>
#include <nano/time.h>

//______________________________________________________________________________
/// Blocks the kernel until events arrive and get served.
//______________________________________________________________________________
void
archKernelBlock(void)
{

    timeOneShotSet(1000000);

    //ulong flags;
    //__save_flags(flags);

    BUG_ON(xenScheduleBlock() < 0);

    //__restore_flags(flags);
}
