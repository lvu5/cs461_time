//-------------------------------------------------------------------------
// Include file for using gdbRead and gdbWrite (syscall.c) functions (edb) 
// And the breakpoint management function (traps.c)                        
// October 2013 Fernando Visca                                             
//-------------------------------------------------------------------------

#include <nano/ethosTypes.h>
//#include <nano/event.h>

Status edbBreakpointMgmt();   // Used in traps.c
Status gdbRead(RdId processId, Event *);   // Called from syscallRead()
Status gdbWrite(RdId processId, String *, Event *);   // Called from syscallWrite()
