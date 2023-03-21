#ifndef CPU_PRIVILEGED_H
#define CPU_PRIVILEGED_H

#if defined(__x86_32__)
#include <nano/x86_32/cpuPrivileged.h>
#elif defined(__x86_64__)
#include <nano/x86_64/cpuPrivileged.h>
#else
#error Unsupported architecture.
#endif

#ifndef __ASSEMBLY__

extern const uint DefaultEflags;
extern const uint ExecStackSize;

typedef enum 
{
    PRIVILEGED_MODE,
    NON_PRIVILEGED_MODE  
} ArchPrivilegeState;

// Returns the arg_number'th system call parameter in *where.
int archSyscallArg(arch_interrupt_regs_t *regs, unsigned arg_number, uintptr_t *where);

// Returns the cpu priviledge state implied by regs in state.
ArchPrivilegeState archPrivilegeState(arch_interrupt_regs_t *regs);

// Sets the system call Status return value.
int archSyscallStatus(arch_interrupt_regs_t *regs, int retval);

// No SMP =).
#define smp_processor_id() 0
void archInit(start_info_t *si);
void archPrintInfo(void);

// Defined in archProcess.c
void archFpuClearDetect(void);
void archFpuSetDetect(void);

#endif
#endif	//CPU_PRIVILEGED_H
