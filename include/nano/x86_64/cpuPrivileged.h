// CPU odds and ends.
// Mar-2008: Andrei Warkentin

#ifndef x86_64_CPU_PRIVILEGED_H
#define x86_64_CPU_PRIVILEGED_H

#include <nano/x86_64/cpu.h>
#include <xen/xen.h>
#include <xen/arch-x86/xen-x86_64.h>

#define __KERNEL_CS  FLAT_KERNEL_CS
#define __KERNEL_DS  FLAT_KERNEL_DS
#define __KERNEL_SS  FLAT_KERNEL_SS

#define TRAP_divide_error      0
#define TRAP_debug             1
#define TRAP_nmi               2
#define TRAP_int3              3
#define TRAP_overflow          4
#define TRAP_bounds            5
#define TRAP_invalid_op        6
#define TRAP_no_device         7
#define TRAP_double_fault      8
#define TRAP_copro_seg         9
#define TRAP_invalid_tss      10
#define TRAP_no_segment       11
#define TRAP_stack_error      12
#define TRAP_gp_fault         13
#define TRAP_page_fault       14
#define TRAP_spurious_int     15
#define TRAP_copro_error      16
#define TRAP_alignment_check  17
#define TRAP_machine_check    18
#define TRAP_simd_error       19
#define TRAP_deferred_nmi     31

#ifndef __ASSEMBLY__

#include <nano/xenEventHandler.h>

// L1 cache size.
#define L1_CACHE_BYTES          128

#define DEFAULT_EFLAGS (0x0202)
#define SEGMENT_MASK (0xFFFF) 


// This is how the pagefault fixup structure looks like.
typedef struct pagefault_fixup_entry_s
{
  uint64_t fault;  // Faulting address.
  uint64_t fixup;  // Address of fixup code.
} pagefault_fixup_entry_t;

// Error code for page fault exception looks like:
#define ARCH_PF_ERROR_P_BIT (0)
#define ARCH_PF_ERROR_W_BIT (1)
#define ARCH_PF_ERROR_U_BIT (2)

// Integral data size.
#define ARCH_INTEGRAL_SIZEOF (sizeof(uint64_t))

// Some accessors to the arch_interrupt_regs_s fields.
#define ArchRegsIp(regs) ((regs)->rip)
#define ArchRegsFp(regs) ((regs)->rbp)

typedef struct arch_interrupt_regs_s {
    unsigned long r15;
    unsigned long r14;
    unsigned long r13;
    unsigned long r12;
    unsigned long rbp;
    unsigned long rbx;
    unsigned long r11;
    unsigned long r10;
    unsigned long r9;
    unsigned long r8;
    unsigned long rax;
    unsigned long rcx;
    unsigned long rdx;
    unsigned long rsi;
    unsigned long rdi;
	unsigned long pad; // Pad to multiple of 16 bytes
    unsigned long error_code;
    unsigned long rip;
    unsigned long cs;
    unsigned long eflags;
    unsigned long rsp;
    unsigned long ss;
} arch_interrupt_regs_t;

/* This is what the memory at a frame pointer looks like */
typedef struct arch_reverse_frame_s
{
  uint64_t old_frame;
  uint64_t return_address;
  uint64_t last_argument;
} arch_reverse_frame_t;

// Some accessors to the arch_reverse_frame_s fields.
#define ARCH_REVF_OLD_FRAME(revf) ((revf)->old_frame)
#define ARCH_REVF_RETURN_ADDRESS(revf) ((revf)->return_address)
#define ARCH_REVF_REGS_FRAME(revf) ((revf)->last_argument)

#define ARCH_FPU_STATE_ALIGN 16

/*
 * These are assembler stubs in entry.S.
 * They are the actual entry points for virtual exceptions.
 */
void entryDivideError(void);
void entryDebug(void);
void entryInt3(void);
void entryOverflow(void);
void entryBounds(void);
void entryInvalidOp(void);
void entryDeviceNotAvailable(void);
void entryCoprocessorSegmentOverrun(void);
void entryInvalidTSS(void);
void entrySegmentNotPresent(void);
void entryStackSegment(void);
void entryGeneralProtection(void);
void entryPageFault(void);
void entryCoprocessorError(void);
void entrySimdCoprocessorError(void);
void entryAlignmentCheck(void);
void entrySpuriousInterruptBug(void);
void entryMachineCheck(void);
void entrySyscall(void);
void trapInit(void);
extern void *entryException;
extern void *entryExceptionEnd;
extern void *hypervisor_callback;
extern void *hypervisor_callback_end;

// Return TRUE if page fault was caused by a present page.
// false means the page is not present (i.e. we had a protection fault).
static inline
bool
ArchRegsPageFaultPresent(arch_interrupt_regs_t *regs)
{
    return regs->error_code & (1 << ARCH_PF_ERROR_P_BIT);
}

// Map page fault error into permission.
static inline
int
ArchRegsPageFaultPerm(arch_interrupt_regs_t *regs)
{
    return (regs->error_code & (1 << ARCH_PF_ERROR_W_BIT)) ? PERM_WRITE : PERM_READ;
}


// Read faulting address which resulted in a page fault.
#define cpu_mm_fault_address() \
          ((vaddr_t) (HYPERVISOR_shared_info->vcpu_info[smp_processor_id()].arch.cr2))

static inline
uint64
getTsc(void) 
{
    uint64 tsc;
    __asm__ __volatile__(
			 "rdtsc\n"
			 "movl %%eax, (%0)\n"
			 "movl %%edx, 4(%0)\n" : : "r" (&tsc) : "eax", "edx", "memory"
			 );
    return tsc;
}

#define wrmsr(msr,val1,val2) \
      __asm__ __volatile__("wrmsr" \
                           : /* no outputs */ \
                           : "c" (msr), "a" (val1), "d" (val2))

#define wrmsrl(msr,val) wrmsr(msr,(u32)((u64)(val)),((u64)(val))>>32)

#endif
#endif //x86_64_CPU_PRIVILEGED_H
