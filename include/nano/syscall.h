#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include <ethos/kernel/common.h>
#include <ethos/kernel/process.h>
#include <ethos/kernel/timer.h>

Status     syscallAdvertise(Fd serviceFd, Ref* serviceName, EventId* eventId);
Status     syscallAuthenticate(void);
Status     syscallBeep(Time64 time, EventId* eventId);
Status     syscallBlock(EventWaitTreeSpec *ewt, uint32 elmts);
Status     syscallBlockAndRetire(EventId eventId, Fd* retiredFd, Ref** retiredValue);
Status     syscallCancel(EventId eventId);
Status     syscallClose(Fd fd);
Status     syscallCopyFd(Fd to, Fd from);
Status     syscallCreateDirectory(Fd dirFd, String* fileName, String* label, HashRef* typeHash, EventId* eventId);
Status     syscallExec(Fd dirFd, Ref* fileName, arch_interrupt_regs_t *regs);
Status     syscallExit(Status exitStatusUserSpace);
Status     syscallFdReceive(EventId* eventId);
Status     syscallFdSend(Ref* fds, Ref* name, Ref* virtualProcess);
Status     syscallFileInformation(Fd dirFd, String* fileName, EventId* eventId);
Status     syscallFork(ulong level, arch_interrupt_regs_t *regs, Fd* terminateFd);
Status     syscallGetCompletedEvents(Ref** completedEvents);
Status     syscallGetNextName(Fd dirFd, Ref* name, EventId* eventId);
Status     syscallGetPendingEvents(Ref** pendingEvents);
ProcessId  syscallGetPid(void);
Status     syscallGetProcessExitStatus(Fd terminateFd, EventId* eventId);
Status     syscallGetProcessGroups(Fd terminateFd, ProcessGroup **processGroups);
Time64     syscallGetTime(void);
Status     syscallGetUser(Ref** userName);
Status     syscallImport(Fd listeningFd, EventId* eventId);
Status     syscallIpc(Fd serviceFd, Ref* serviceName, Ref* toMachine, Ref* writeValue, EventId* eventId);
Status     syscallKill(Fd terminateFd);
Status     syscallKillProcessGroup(ProcessId processGroupId, Ref** returnRef);
Status     syscallOpenDirectory(Fd dirFd, String* fileName, EventId* eventId);
Status     syscallPeek(Fd peekFd, EventId *eventId);
Status     syscallPrint(String* printString);
Status     syscallRandom(uint32 size, EventId *eventId);
Status     syscallRead(Fd fd, EventId* eventId);
Status     syscallReadVar(Fd dirFd, String* fileName, EventId *eventId);
Status     syscallRemoveDirectoryOrFile(Syscall syscall, Fd dirFd, String* fileName, EventId *eventId);
Status     syscallRetire(EventId eventId, Fd* retiredFd, Ref** retiredValue);
Status     syscallShutdown(void);
Status     syscallSign(Fd dirFd, String* fileName, EventId* eventId);
Status     syscallWrite(Fd fd, Ref* contents, EventId *eventId);
Status     syscallWriteVar(Fd dirFd, String* fileName, Ref* contents, EventId *eventId);

#endif
