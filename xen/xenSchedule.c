//_________________________________________________________________________
/// Xen scheduling hypercalls
//
// 
// (C) 2009 - Jon A. Solworth
//_________________________________________________________________________

#include <nano/common.h>
#include <nano/mm.h>
#include <nano/xenEventHandler.h>
#include <nano/xenSchedule.h>


//_________________________________________________________________________
/// Need to wait for other OS.
//_________________________________________________________________________
int
xenScheduleBlock()
{
  return HYPERVISOR_sched_op(SCHEDOP_block, 0);
}

//_________________________________________________________________________
/// Need to wait for other OS.
//_________________________________________________________________________
int
xenScheduleYield()
{
  return HYPERVISOR_sched_op(SCHEDOP_yield, 0);
}

//_________________________________________________________________________
/// shutdown OS
//_________________________________________________________________________
int
xenScheduleShutdown(int shutdownType)
{
  sched_shutdown_t shutdown;
  shutdown.reason = SHUTDOWN_poweroff;
  return HYPERVISOR_sched_op(SCHEDOP_shutdown, &shutdown);
}
