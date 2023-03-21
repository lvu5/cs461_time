//______________________________________________________________________________
/// Event management for processes
//______________________________________________________________________________

#ifndef __EVENT_PROCESS_H__
#define __EVENT_PROCESS_H__

#include <ethos/kernel/process.h>

// On an exec or exit, removes all events and destroys any wait trees
Status eventProcessCleanup(Process*);

Status eventProcessDestroyCompleted(Process *process);

#endif //__EVENT_PROCESS_H__
