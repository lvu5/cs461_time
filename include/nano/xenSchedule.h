//______________________________________________________________________________
/// Xen event handling
//
// (C) 2009 - Jon A. Solworth
//______________________________________________________________________________

#include <xen/sched.h>

#ifndef __XEN_SCHEDULE_H__
#define __XEN_SCHEDULE_H__

int xenScheduleBlock(void);
int xenScheduleYield(void);
int xenScheduleShutdown(int);
int xenKernelBlock(void);

#endif
