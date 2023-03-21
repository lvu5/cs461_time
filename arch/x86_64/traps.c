//______________________________________________________________________________
/// CPU traps.
// Mar-2008: Andrei Warkentin
//______________________________________________________________________________

#include <nano/common.h>
#include <nano/cpuPrivileged.h>
#include <nano/mm.h>
#include <nano/debug.h>
#include <nano/archDebug.h>
#include <nano/fmt.h>

void archFpuStateSave(arch_fpu_state_t *state);
void archFpuStateRestore(arch_fpu_state_t *state);

//______________________________________________________________________________
/// The default trap hanlder
/// traps which are not (currently) handled.
/// When they occur in the kernel, kernel exits.
//  When they occur in user space, the process is terminated
//______________________________________________________________________________
static
void 
trapDefault(int trapNumber,                            ///< the trap number
	    char *str,                                 ///< a message string
	    arch_interrupt_regs_t *regs                ///< registers at time of trap
	    )
{
    ArchPrivilegeState state = archPrivilegeState(regs);
    if (state == PRIVILEGED_MODE)
	{
	    printfLog("Unhandled trap %d (%s) in kernel\n", trapNumber, str);
	    debugExit(regs);
	}
    else
	{
	    printfLog("Unprivileged %s fault (trap %d) - terminated\n", 
		   str, trapNumber);
	    archDebugRegsDump2(regs);
	}
}

//______________________________________________________________________________
// Debugger breakpoint trap
//______________________________________________________________________________
void
trapInt3(arch_interrupt_regs_t *regs)
{
    ArchPrivilegeState state = archPrivilegeState(regs);
    if (state == PRIVILEGED_MODE)
	{
	    printfLog("Unhandled trap (Int3) in kernel\n");
	    debugExit(regs);
	}
    else
	{
	  printfLog("Unprivilege breakpoint trap\n");
	  BUG();
	}
}

//______________________________________________________________________________
/// Implements lazy saving of FP/MMX registers.
/// ONLY occurs when an FPU access is made & CR0.TS is set.
//______________________________________________________________________________
void
trapDeviceNotAvailable(arch_interrupt_regs_t *regs)
{
  printfLog("lazy saving of FP/MMX not implemented\n");
  archDebugRegsDump(regs);

  BUG();
}

//______________________________________________________________________________
/// create C trap handler for each trap described by TRAP_DEFAULT macro
//______________________________________________________________________________
#define TRAP_DEFAULT(trapNumber, str, name) \
void trap##name(struct arch_interrupt_regs_s *regs) \
{ \
	trapDefault(trapNumber, str, regs); \
}

TRAP_DEFAULT( 0, "divide error", DivideError)
TRAP_DEFAULT( 1, "debug", Debug)
//TRAP_DEFAULT( 3, "int3", Int3)
TRAP_DEFAULT( 4, "overflow", Overflow)
TRAP_DEFAULT( 5, "bounds", Bounds)
TRAP_DEFAULT( 6, "invalid operand", InvalidOp)
TRAP_DEFAULT( 9, "coprocessor segment overrun", CoprocessorSegmentOverrun)
TRAP_DEFAULT(10, "invalid TSS", InvalidTSS)
TRAP_DEFAULT(11, "segment not present", SegmentNotPresent)
TRAP_DEFAULT(12, "stack segment", StackSegment)
TRAP_DEFAULT(13, "general protection", GeneralProtection)
TRAP_DEFAULT(17, "alignment check", AlignmentCheck)
TRAP_DEFAULT(18, "machine check", MachineCheck)
TRAP_DEFAULT(15, "spurious interrupt\n", SpuriousInterruptBug)
TRAP_DEFAULT(16, "coprocessor error\n", CoprocessorError)
TRAP_DEFAULT(19, "SIMD coprocessor error\n", SimdCoprocessorError)


//______________________________________________________________________________
// Submit a virtual IDT to the hypervisor. This consists of tuples
// (interrupt number, privilege ring, CS:EIP of handler).
// The 'privilege ring' field specifies the least-privileged ring that
// can trap to that vector using a software-interrupt instruction (INT).
//______________________________________________________________________________
static trap_info_t trapTable[] = {
    {    0, 0, __KERNEL_CS, (ulong)entryDivideError               },
    {    1, 0, __KERNEL_CS, (ulong)entryDebug                     },
    {    3, 3, __KERNEL_CS, (ulong)entryInt3                      },
    {    4, 3, __KERNEL_CS, (ulong)entryOverflow                  },
    {    5, 3, __KERNEL_CS, (ulong)entryBounds                    },
    {    6, 0, __KERNEL_CS, (ulong)entryInvalidOp                 },
    {    7, 0, __KERNEL_CS, (ulong)entryDeviceNotAvailable        },
    {    9, 0, __KERNEL_CS, (ulong)entryCoprocessorSegmentOverrun },
    {   10, 0, __KERNEL_CS, (ulong)entryInvalidTSS                },
    {   11, 0, __KERNEL_CS, (ulong)entrySegmentNotPresent         },
    {   12, 0, __KERNEL_CS, (ulong)entryStackSegment              },
    {   13, 0, __KERNEL_CS, (ulong)entryGeneralProtection         },
    {   14, 0, __KERNEL_CS, (ulong)entryPageFault                 },
    {   15, 0, __KERNEL_CS, (ulong)entrySpuriousInterruptBug      },
    {   16, 0, __KERNEL_CS, (ulong)entryCoprocessorError          },
    {   17, 0, __KERNEL_CS, (ulong)entryAlignmentCheck            },
    {   19, 0, __KERNEL_CS, (ulong)entrySimdCoprocessorError      },
    { 0x80, 3, __KERNEL_CS, (ulong)entrySyscall                   },
    {    0, 0,           0, 0                                     }
};   

//______________________________________________________________________________
/// Tells xen about the trap table
//______________________________________________________________________________
void
trapInit(void)
{
    HYPERVISOR_set_trap_table(trapTable);    
}
