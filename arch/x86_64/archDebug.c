//______________________________________________________________________________
/// Architecture-specific debug code.
// Mar-2008: Andrei Warkentin
//______________________________________________________________________________

#include <nano/common.h>
#include <nano/debug.h>
#include <nano/console.h>

//
// Returns true if the frame for function corresponding to
// returnAddress is actually an interrupt context structure.
//
bool 
archDebugUnwindNextFrameIsRegs(vaddr_t returnAddress)
{
    return ((returnAddress >= (vaddr_t) &entryException && returnAddress < (vaddr_t) &entryExceptionEnd) ||
	    (returnAddress >= (vaddr_t) &hypervisor_callback && returnAddress < (vaddr_t) &hypervisor_callback_end));
}

struct pt_regs;

void
archDebugRegsDump2(arch_interrupt_regs_t *regs)
{
#ifdef __x86_32__    
    printfLog("EIP: %x, EFLAGS %x.\n", regs->eip, regs->eflags);
    printfLog("EBX: %08x ECX: %08x EDX: %08x\n",
		regs->ebx, regs->ecx, regs->edx);
    printfLog("ESI: %08x EDI: %08x EBP: %08x EAX: %08x\n",
		regs->esi, regs->edi, regs->ebp, regs->eax);
    printfLog("DS: %04x ES: %04x  eip: %08x\n",
		regs->xds, regs->xes, regs->eip);
    printfLog("CS: %04x EFLAGS: %08x ss: %04x\n",
		regs->xcs, regs->eflags, regs->xss);
#else
    printfLog("RIP: %04lx:[<%016lx>] ", regs->cs & 0xffff, regs->rip);
    printfLog("\nRSP: %04lx:%016lx  EFLAGS: %08lx\n", 
		regs->ss, regs->rsp, regs->eflags);
    printfLog("RAX: %016lx RBX: %016lx RCX: %016lx\n",
		regs->rax, regs->rbx, regs->rcx);
    printfLog("RDX: %016lx RSI: %016lx RDI: %016lx\n",
		regs->rdx, regs->rsi, regs->rdi); 
    printfLog("RBP: %016lx R08: %016lx R09: %016lx\n",
		regs->rbp, regs->r8, regs->r9); 
    printfLog("R10: %016lx R11: %016lx R12: %016lx\n",
		regs->r10, regs->r11, regs->r12); 
    printfLog("R13: %016lx R14: %016lx R15: %016lx\n",
		regs->r13, regs->r14, regs->r15); 
#endif
}

//______________________________________________________________________________
/// dumps out the registers
//______________________________________________________________________________
Status
archDebugRegsDump(arch_interrupt_regs_t *regs)
{
    vaddr_t ip, actual_symbol_address = 0;
    char *name = NULL;

    ArchPrivilegeState state = archPrivilegeState(regs);
    printfLog("Dumping regs structure at 0x%p\n", regs);

#ifdef __x86_32__
    ip = (vaddr_t)regs->eip;
    printfLog("EIP: %04x:%08x", regs->xcs & 0xffff, regs->eip);
#elif defined(__x86_64__)
    ip = (vaddr_t)regs->rip;
    printfLog("RIP: 0x%p", regs->rip);
#endif

    if (state == PRIVILEGED_MODE && 
	debugLookupAddress(ip, &name, &actual_symbol_address) == StatusOk)
	{
	    printfLog(" <%s + 0x%x>", name, ip - actual_symbol_address);
	}

#ifdef __x86_32__
    printfLog("\nEAX: 0x%p EBX: 0x%p ECX: 0x%p EDX: 0x%p\n", regs->eax,
		regs->ebx, regs->ecx, regs->edx);
    printfLog("ESI: 0x%p EDI: 0x%p EBP: 0x%p ESP: 0x%p\n",
		regs->esi, regs->edi, regs->ebp, regs->xesp);
    printfLog("DS: 0x%04x\nES: 0x%04x\nFS: 0x%04x\nGS: 0x%04x\n",
		regs->xds, regs->xes, regs->xfs, regs->xgs);
    printfLog("EFLAGS: 0x%p\nException error code: %04x\n",
		regs->eflags, regs->error_code);
#elif defined(__x86_64__)
    printfLog("\nRAX: 0x%p RBX: 0x%p RCX: 0x%p RDX: 0x%p\n", regs->rax,
		regs->rbx, regs->rcx, regs->rdx);
    printfLog("RSI: 0x%p RDI: 0x%p RBP: 0x%p RSP: 0x%p\n",
		regs->rsi, regs->rdi, regs->rbp, regs->rsp); 
    printfLog("R8:  0x%p R9:  0x%p R10: 0x%p R11: 0x%p\n", regs->r8,
		regs->r9, regs->r10, regs->r11);
    printfLog("R12: 0x%p R13: 0x%p R14: 0x%p R15: 0x%p\n", regs->r12,
		regs->r13, regs->r14, regs->r15);
    printfLog("RFLAGS: 0x%p\nException error code: %04x, SS: 0x%p, CS: 0x%p\n",
		regs->eflags, regs->error_code, regs->ss, regs->cs);
#endif

    return StatusOk;
}
