//______________________________________________________________________________
/// latch.h: latch code
//______________________________________________________________________________

#include <ethos/kernel/common.h>
#include <ethos/kernel/debug.h>
#include <ethos/kernel/arch/archDebug.h>

#ifndef __LATCH_H__
#define __LATCH_H__

typedef struct latch_s {
    int               busy;           // non-zero if busy
    ListHead  waitingEvents;  // list of waiting events
} Latch;

static
inline
void
latchInitialize(Latch *latch)
{
    latch->busy = 0;
    INIT_LIST_HEAD(&latch->waitingEvents);
}


static
inline
int
latchIsFree(Latch *latch)
{
    return !latch->busy;
}

static
inline
void
latchObtain(Latch *latch, Event *event)
{
    ASSERT(latch);
    ASSERT(event);
    debugXprint(debugLatch, "latchObtain: latch->busy=$[int]\n", latch->busy);
    if (latch->busy)
	{
	    list_add_tail(&event->waitingList, &latch->waitingEvents);
	    eventBlock(event);
	}
    latch->busy = 1;
}

static
inline
void
latchRelease(Latch *latch)
{
    ASSERT(latch);
    latch->busy = 0;
    debugXprint(debugLatch, "latchRelease: list_empty=$[int]\n", list_empty(&latch->waitingEvents));
    if (!list_empty(&latch->waitingEvents))
	{
	    ListHead *elmt = latch->waitingEvents.next;
	    Event *event = list_entry(elmt, Event, waitingList);
	    list_del_init(elmt);
	    eventUnblock(event);

	    // busy once again
	    latch->busy = 1;
	}
}
#endif
