//______________________________________________________________________________
// Process management.
// Mar-2008: Andrei Warkentin
//______________________________________________________________________________

#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <ethos/ethosTypes.h>
#include <ethos/fdType.h>
#include <ethos/list.h>
#include <ethos/ref.h>
#include <ethos/net/auth.h>
#include <ethos/kernel/time.h>
#include <ethos/kernel/mm_public.h>
#include <ethos/kernel/arch/schedPrivileged.h>
#include <ethos/kernel/processMemory.h>
#include <ethos/execContext.h>
#include <ethos/net/connection.h>

enum {
    SchedulingQuantum = 1000,
};

// Name of the first process.  Used only at system boot
#define    InitialProcessName "init"
extern String *initContents;

typedef struct Process Process;

typedef ProcessId (ProcessGroup)[MaxProcessGroup];

// Current process with state RUNNING.
Process *current;

// Last process to use the FPU
extern Process *floatingPointLastProcess;

typedef struct arch_process_s
{
    uchar  *user_stack;        // Stack used when actual process code is executing.
} arch_process_t;

typedef struct FdTableS
{
    FdType    fdType; // type of file descriptor
    RdId      fdId;   // points to the fdid if allocated file,
                      // points to the Network if allocated network/IPC,
                      // points to the process if debug or terminate portal
                      // 0 if unallocated
    Ref      *fdRef;  // last file name for streaming reads of directories
                      // exit value for terminate portal
    Fd        next;   // to the next on the free list, FdTableSize otherwise
} FdTable;

typedef struct { // fds to be sent to a virtual process
    RdId        fdId;
    FdType      fdType;
    Ref        *fdRef;
    ListHead    pendingList;
} PendingFd;  

typedef struct FloatingPointStore {
   arch_fpu_state_t *state;    // NULL if no store has been allocated
   int saved;                  // positive if state has been saved, 0 otherwise           
} FloatingPointStore;

//_______________________________________________________________
// Data structure to be stored in process control block. Used
// by EDB for debugging purposes.
//
// Feb 2014: Fernando Visca
// ______________________________________________________________
typedef struct EdbProcessData {
    // Status to return ( see EdbRpcDebug )
    Status edbStatus;

    // Pid of the process being debugged
    ProcessId debugPid;

    // Is this process being debugged by another process?
    uchar debuggedFlag;

    // EventIds used for the attach procedure
    EventId gdbProxyEventId;
    EventId trapInt3EventId;

    // Reply - used by debug RPC
    GdbProxyReply edbReply;
} EdbProcessData;

//______________________________________________________________________________
// Definition of process context
//______________________________________________________________________________
typedef struct Process
{
    // contains information for kernel execution environment
    ExecContext     execContext;

    // Architecture specific state (registers, etc)
    arch_process_t specific;
  
    // Floating point store
    FloatingPointStore fpuStore;

    // ref to Userspace, kernel exit status
    // this is ref is copied over to file descriptors that reference it
    Ref *exitStatus;

    // List of all events which are waiting on this process to exit
    ListHead exitEventWaiting;

    // Events which are not yet completed.
    ListHead pendingEvents;

    // Completed Events.
    ListHead completedEvents;

    // number of pendingEvents plus completedEvents
    msize_t eventCount; 

    // Used for keeping track of process within the ready,
    // waiting and terminated lists.
    ListHead processList;
 
    // Address Space Context.
    ProcessMemory *processMemory;

    // Resource list.
    ListHead resourceList;

    // Fd table
    FdTable fdTable[FdTableSize];

    // Next free slot in fd table
    Fd fdNextFree;

    // Deschedule time. Used by the scheduler to figure out whether
    // a process had run over its allotted quantum. During scheduling,
    // reset to to current time + quantum legth. The quantum comprises
    // both the time spent in the kernel and in the program code.
    Time64 descheduleTime;

    // User that owns process.
    AuthUser *user;

    // Process ID.
    ProcessId   processId;

    // Process group ID
    ProcessGroup  processGroups;

    // Program's name
    String *programName;

    // user space debugging
    EdbProcessData edbProcessData;
} Process;

// used for loading per-user executable
RdId PerUserExecutableRdId;

// Initializes process management and starts the first process.
void scheduleInit(void);

// Invoked in interrupt prologue. Enters the main scheduler 
// logic that sees if there is a need to deschedule.
void scheduleEnter(
		   arch_interrupt_regs_t *regs
		   );

// Clones the current process.
Status processClone(
		    arch_interrupt_regs_t *regs,
		    ulong levels,
		    Process **childProcess,
		    Fd *terminateFd
		    );

void  processInit(void);

// create the initial or virtual orphaned process
Status processOrphanInit(AuthUser *authUser);

// create a virtual process
Status processCreateVirtual(String* executable, AuthUser *user, Process **returnProcess);

// Remove a virtual process from a user's virtual process list
void virtualProcessExit(void);




// Print a process
void processPrint(void *ptr);

// Kills a process from kernel
void processKill(Process *process);

// Replaces the current process's executable
Status processExec(
		   arch_interrupt_regs_t *regs,
		   RdId dirRdId,
		   String *name
		   );

Status
processFind(
	    ProcessId processId,
	    Process **process
	    );

// Blocks the current process, scheduling to the next.
void processBlock(void);

// Unblock the given process.
void processUnblock(Process *process);

// Exits the current process, scheduling to the next.
void processExit(Process *process, ExitStatus exitStatus);

Status processTerminateRead(ProcessId processId, Event *event);

Status virtualProcessFdReceive(Event *event);
Status virtualProcessFdSend(ListHead *pendingFdHead, AuthUser *user, String *virtualProcess);

Status fdTableReserve(Fd *fd);

Status fdTableSet(Fd fd, RdId fdId, FdType fdType, Ref *fdRef);

Status fdClose(Process *process, Fd fd);

Status fdGetLastFileName(ProcessId processId, Fd fd, Ref ***returnLastFileName);

Status fdFind(Fd fd, FdType *fdType, RdId *returnFdId);
Status fdFindAll(Fd fd, FdType *fdType, RdId *returnFdId, Ref **returnRef);

Status fdFindDirectory(Fd fd, RdId *returnFdId);

Status fdTableInit(Process*);

Status fdDecrement(RdId fdId, FdType fdType);

Status fdIncrement(RdId fdId, FdType fdType);

Status fdTableClean(Process *process);

Status fdTableCopy(Process *to, Process *from);

Status fdCopy(Fd toFd, Fd fromFd);

void    fdRefUnhook(Process *process, Fd fd);

void floatingPointStateSave(void);

void floatingPointStateRestore(void);

void floatingPointFree(Process *process);

void floatingPointInit(void);
#endif /* __PROCESS_H__ */
