//______________________________________________________________________________
// System timer.
// May-2008: Andrew Trumbo
//______________________________________________________________________________

#ifndef __TIMER_H__
#define __TIMER_H__

#include <nano/common.h>
#include <nano/time.h>


// Loops through timers and deal with the expired ones.
void timerHandlerDeferred(void);

// Initialize whats needed for timers to function.
void timerInit(void);

#endif /* __TIMER_H__ */
