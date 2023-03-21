/*______________________________________________________________________________
 * Nano kernel x86 64-bit assembly code
 *
 * _start
 * entryException
 * hypervisor_callback
 * entryProcessStart, entryEventStart
 * entryContextSwitch
 * archFpuClearDetect, archFpuStateSave, archFpuStateRestore
 * userspace_[8|4|2|1][from|to]_nocheck
 * userspace_memcpy_nocheck
 *______________________________________________________________________________
 */

#include <nano/cpuPrivileged.h>
#include <nano/status.h>
#include <nano/macros.h>
#include <xen/features.h>


/* Tell Xen about kernel */
.section __xen_guest
	.ascii	"GUEST_OS=NanoOS"
	.ascii	",XEN_VER=xen-3.0"
	.ascii	",VIRT_BASE=0xffffffff80000000"
	.ascii	",ELF_PADDR_OFFSET=0xffffffff80000000"
	.ascii  ",VIRT_ENTRY=0xffffffff80000000"
	.ascii	",HYPERCALL_PAGE=0x2"
	.ascii	",BSD_SYMTAB=yes"
	.ascii	",LOADER=generic"
	.byte	0
.text


.code64

#define ENTRY(X) .globl X ; X :
.globl _start, shared_info, hypercall_page

/* Xen begins executing Ethos here */
BEGIN_FUNCTION(_start)
        cld
	movabsq $stack_start, %rsp
        movq (%rsp), %rsp            /* Set up stack */
        movq %rsi, %rdi              /* Send start_info to the right place */
	movabsq $startKernel, %rax
        call *%rax                   /* Go. */
END_OBJECT(_start)

stack_start:
        .quad stack+8192

        /* Unpleasant -- the PTE that maps this page is actually overwritten */
        /* to map the real shared-info page! :-)                             */
        .org 0x1000
shared_info:                         /* Xen shared info page */

        .org 0x2000
hypercall_page:                      /* Page contains hypercalls mapped by Xen */

        .org 0x3000
die:	xor %rdi, %rdi
	jmp debugExit

/* Offsets into shared_info_t. */                
#define evtchn_upcall_pending 0
#define evtchn_upcall_mask    1

NMI_MASK = 0x80000000
TS_MASK  = 0x8
KERNEL_CS_MASK = 0xfc

/* Offsets into arch_interrupt_regs_t */
#define RFLAGS    152
#define RIP       136
#define RAX       80
#define CS        144
#define RSP       160
#define ORIG_RAX  128

#define REST_SKIP 6*8			
.macro SAVE_REST
	subq $REST_SKIP,%rsp
	movq %rbx,5*8(%rsp) 
	movq %rbp,4*8(%rsp) 
	movq %r12,3*8(%rsp) 
	movq %r13,2*8(%rsp) 
	movq %r14,1*8(%rsp) 
	movq %r15,(%rsp) 
.endm		


.macro RESTORE_REST
	movq (%rsp),%r15
	movq 1*8(%rsp),%r14
	movq 2*8(%rsp),%r13
	movq 3*8(%rsp),%r12
	movq 4*8(%rsp),%rbp
	movq 5*8(%rsp),%rbx
	addq $REST_SKIP,%rsp
.endm


#define ARG_SKIP 9*8
.macro RESTORE_ARGS addskip=0
	movq (%rsp),%r11
	movq 1*8(%rsp),%r10
	movq 2*8(%rsp),%r9
	movq 3*8(%rsp),%r8
	movq 4*8(%rsp),%rax
	movq 5*8(%rsp),%rcx
	movq 6*8(%rsp),%rdx
	movq 7*8(%rsp),%rsi
	movq 8*8(%rsp),%rdi
	.if ARG_SKIP+\addskip > 0
	addq $ARG_SKIP+\addskip,%rsp
	.endif
.endm

/* Must move through a register due to reloc. constraints */
#define XEN_GET_VCPU_INFO(reg)	movabsq $HYPERVISOR_shared_info,reg;\
                                movq (reg), reg
#define XEN_PUT_VCPU_INFO(reg)
#define XEN_PUT_VCPU_INFO_fixup
#define XEN_LOCKED_BLOCK_EVENTS(reg)	movb $1,evtchn_upcall_mask(reg)
#define XEN_LOCKED_UNBLOCK_EVENTS(reg)	movb $0,evtchn_upcall_mask(reg)
#define XEN_TEST_PENDING(reg)	testb $0xFF,evtchn_upcall_pending(reg)

#define XEN_BLOCK_EVENTS(reg)	XEN_GET_VCPU_INFO(reg)			; \
                    			XEN_LOCKED_BLOCK_EVENTS(reg)	; \
    				            XEN_PUT_VCPU_INFO(reg)

#define XEN_UNBLOCK_EVENTS(reg)	XEN_GET_VCPU_INFO(reg)			; \
                				XEN_LOCKED_UNBLOCK_EVENTS(reg)	; \
    			            	XEN_PUT_VCPU_INFO(reg)


.macro hypervisor_iret flag
	testb $3,1*8(%rsp)         # check for userspace CS
	jnz   2f
	testl $NMI_MASK,2*8(%rsp)  # check eflags for NMI
	jnz   2f

	/* Direct iret to kernel space. Correct CS and SS (ring 3, x86_64) */
	orb   $3,1*8(%rsp)            # set ring 3 on CS
	orb   $3,4*8(%rsp)            # set ring 3 on SS
1:	iretq

2:	/* Slow iret via hypervisor. (userspace/NMI) */
	andl  $~NMI_MASK, 16(%rsp)    # zero out NMI bit
	pushq $\flag
	jmp  hypercall_page + (__HYPERVISOR_iret * 32)
.endm

.macro zeroentry sym
	movq (%rsp),%rcx
	movq 8(%rsp),%r11
	addq $0x10,%rsp        /* skip rcx and r11 */
	pushq $0	       /* push error code/oldrax */
	pushq $0               /* padding */
	pushq %rax	       /* push real oldrax to the rdi slot */
	leaq \sym(%rip), %rax
	jmp entryException
.endm

.macro errorentry sym
	movq (%rsp),%rcx
	movq 8(%rsp),%r11
	addq $0x10,%rsp        /* rsp points to the error code */
	pushq $0               /* padding */
	pushq %rax
	leaq \sym(%rip),%rax
	jmp entryException
.endm

/*______________________________________________________________________________
 * Exception entry point. This expects an error code/orig_rax on the stack
 * and the exception handler in %rax.
 *______________________________________________________________________________
 */ 		  				
BEGIN_FUNCTION(entryException)
	/* rdi slot contains rax, oldrax contains error code */
	cld	
	subq $14*8,%rsp
	movq %rsi,13*8(%rsp)
	movq 14*8(%rsp),%rsi	/* load rax from rdi slot */
	movq %rdi,14*8(%rsp)	/* store rdi */
	movq %rdx,12*8(%rsp)
	movq %rcx,11*8(%rsp)
	movq %rsi,10*8(%rsp)	/* store rax */ 
	movq %r8, 9*8(%rsp)
	movq %r9, 8*8(%rsp)
	movq %r10,7*8(%rsp)
	movq %r11,6*8(%rsp)
	movq %rbx,5*8(%rsp) 
	movq %rbp,4*8(%rsp) 
	movq %r12,3*8(%rsp) 
	movq %r13,2*8(%rsp) 
	movq %r14,1*8(%rsp) 
	movq %r15,(%rsp) 

/* our handlers only take one argument (regs) */
error_call_handler:
	movq %rsp,%rdi              # arg: struct arch_interrupt_regs_t *
	call *%rax                  # call the handler

	jmp schedule_and_disable_fpu
END_OBJECT(entryException)
BEGIN_OBJECT(entryExceptionEnd)  # This is not strictly correct (see jmp error_exit above)
END_OBJECT(entryExceptionEnd)

/*______________________________________________________________________________
 * Xen event entry point.
 *______________________________________________________________________________
 */
ENTRY(hypervisor_callback)
	zeroentry hypervisor_callback_internal

ENTRY(hypervisor_callback_internal)
	movq %rdi, %rsp				# restore rsp

	/* check against re-entrance */
	movq RIP(%rsp),%rax
	cmpq $scrit,%rax
	jb 11f
	cmpq $ecrit,%rax
	jb  critical_region_fixup

11:	call xenEventHandlerCallback

schedule_and_disable_fpu:
	testb $3,CS(%rsp)         # check for kernel space CS
	jz   retint_kernel

	# movq %rsp, %rdi
	# call scheduleEnter

	# movq %cr0, %rax
	# orq  $TS_MASK, %rax
	# movq %rax, %cr0             # set CR0.TS

	# movq %rsp, %rdi
	# call archFpuSetDetect       # check for FPU detect
	# jmp  retint_restore_args	# return to user space via hypercall iret
								# need not re-enable event delivery
retint_kernel:
	movl RFLAGS(%rsp), %eax
	shr $9, %eax                        # EAX[0] == IRET_RFLAGS.IF
	XEN_GET_VCPU_INFO(%rsi)
	andb evtchn_upcall_mask(%rsi),%al
	andb $1,%al                         # EAX[0] == IRET_RFLAGS.IF & event_mask
	jnz restore_all_enable_events       #        != 0 => enable event delivery
	XEN_PUT_VCPU_INFO(%rsi)

retint_restore_args:
	RESTORE_REST
	RESTORE_ARGS 16
	hypervisor_iret 0

restore_all_enable_events:
	RESTORE_REST
	RESTORE_ARGS 16
	pushq %rax                   # save %rax before it is used in critical section
	RSP_OFFSET=8                 # used for restoring %rax
	XEN_GET_VCPU_INFO(%rax)      # safe to use %rax since it is saved
	XEN_UNBLOCK_EVENTS(%rax)

scrit:	/**** START OF CRITICAL REGION ****/
	XEN_TEST_PENDING(%rax)
	XEN_PUT_VCPU_INFO(%rax)
	jz 12f
	XEN_LOCKED_BLOCK_EVENTS(%rax)
12:	popq %rax

restore_end:
	jnz hypervisor_prologue         # safe to jump out of critical region
	                                # because events are masked if ZF != 1
	hypervisor_iret 0
ecrit:  /**** END OF CRITICAL REGION ****/

hypervisor_prologue:
	pushq %r11
	pushq %rcx
	jmp hypervisor_callback

# [How we do the fixup]. We want to merge the current stack frame with the
# just-interrupted frame. How we do this depends on where in the critical
# region the interrupted handler was executing, and so if %rax has been
# restored. We compare interrupted %rip with "restore_end" to determine.
# We always copy a fixed number of of bytes from the current stack frame
# to the end of the previous activation frame so that we can continue 
# as if we've never even reached 11 running in the old activation frame.
critical_region_fixup:
		# Set up source and destination region pointers
		leaq RIP(%rsp),%rsi   # %esi points at end of src region
		# Acquire interrupted rsp which was saved-on-stack. This points to
		# the end of dst region. Note that it is not necessarily current rsp
		# plus 0xb0, because the second interrupt might align the stack frame.
		movq RSP(%rsp),%rdi   # %edi points at end of dst region

		cmpq $restore_end,%rax
		jae  13f

		# If interrupted rip is before restore_end
		# then stack (%rax) hasn't been restored yet
		movq (%rdi),%rax
		movq %rax, RAX(%rsp)  # save rax
		addq $RSP_OFFSET,%rdi

		# Set up the copy
13:		movq $RIP,%rcx
		shr  $3,%rcx          # convert bytes into count of 64-bit entities
15:		subq $8,%rsi          # pre-decrementing copy loop
		subq $8,%rdi
		movq (%rsi),%rax
		movq %rax,(%rdi)
		loop 15b
16:		movq %rdi,%rsp        # final %rdi is top of merged stack
		andb $KERNEL_CS_MASK,CS(%rsp)      # CS might have changed
		jmp  11b
BEGIN_OBJECT(hypervisor_callback_end)
END_OBJECT(hypervisor_callback_end)



ENTRY(failsafe_callback)
        popq  %rcx
        popq  %r11
        iretq


/* Exception handlers */
ENTRY(entryDivideError)
        zeroentry trapDivideError

ENTRY(entryDebug)
        zeroentry trapDebug

ENTRY(entryInt3)
        zeroentry trapInt3

ENTRY(entryOverflow)
        zeroentry trapOverflow

ENTRY(entryBounds)
        zeroentry trapBounds
    
ENTRY(entryInvalidOp)
        zeroentry trapInvalidOp

ENTRY(entryDeviceNotAvailable)
        zeroentry trapDeviceNotAvailable

ENTRY(entryCoprocessorSegmentOverrun)
        zeroentry trapCoprocessorSegmentOverrun

ENTRY(entryInvalidTSS)
        errorentry trapInvalidTSS

ENTRY(entrySegmentNotPresent)
        errorentry trapSegmentNotPresent

/* runs on exception stack ?? */
ENTRY(entryStackSegment)
        errorentry trapStackSegment

ENTRY(entryGeneralProtection)
        errorentry trapGeneralProtection

ENTRY(entryPageFault)
        errorentry pageFaultHandler

ENTRY(entryCoprocessorError)
        zeroentry trapCoprocessorError

ENTRY(entryAlignmentCheck)
        errorentry trapAlignmentCheck

ENTRY(entryMachineCheck)
        zeroentry trapMachineCheck

ENTRY(entrySimdCoprocessorError)
        zeroentry trapSimdCoprocessorError

ENTRY(entrySpuriousInterruptBug)
        zeroentry trapSpuriousInterruptBug            

ENTRY(entrySyscall)
        zeroentry traditionalSyscallHandler

BEGIN_FUNCTION(entryProcessStart)

	# Interrupts could have been disabled if we scheduled here from
	# thread that was pre-empted.
	# FIXME: This always touches vcpu_info[0]. When we go SMP,
	# this will need to be fixed.
	
	# call archFpuSetDetect

	jmp restore_all_enable_events

END_OBJECT(entryProcessStart)

/* void archFpuClearDetect(void) */
BEGIN_FUNCTION(archFpuClearDetect)
	movq %cr0, %rax
	andq $~TS_MASK, %rax
	movq %rax, %cr0
	ret
END_OBJECT(archFpuClearDetect)

/* void archFpuStateSave(arch_fpu_state_t *state) */
BEGIN_FUNCTION(archFpuStateSave)
	fxsave (%rdi)
	finit
	ret
END_OBJECT(archFpuStateSave)

/* void archFpuStateRestore(arch_fpu_state_t *state) */
BEGIN_FUNCTION(archFpuStateRestore)
	fxrstor (%rdi)
	ret
END_OBJECT(archFpuStateRestore)


/* 
		User address space accessors. By reducing the amount of code
		that accesses userspace addresses directly, we can more easily
		separate kernel bugs from invalid addresses passed by programs
		through system calls.
*/

/* int userspace_8from_nocheck(void *address, uint64_t *store) */ 
BEGIN_FUNCTION(userspace_8from_nocheck)
	pushq %rbp
	mov %rsp, %rbp
1:	movq (%rdi), %rax
	movq %rax, (%rsi)
	movabsq $StatusOk, %rax
2:      pop %rbp
	ret
3:      mov $StatusPageFault, %rax
	jmp 2b
.section pagefault_fixups, "a" /* Allocatable */
	.quad 1b, 3b
.previous
END_OBJECT(userspace_8from_nocheck)

/* int userspace_4from_nocheck(void *address, uint32_t *store) */ 
BEGIN_FUNCTION(userspace_4from_nocheck)           
	pushq %rbp
	mov %rsp, %rbp
1:	movl (%rdi), %eax
	movl %eax, (%rsi)
	movabsq $StatusOk, %rax
2:      pop %rbp
	ret
3:      mov $StatusPageFault, %rax
	jmp 2b
.section pagefault_fixups, "a" /* Allocatable */
	.quad 1b, 3b
.previous
END_OBJECT(userspace_4from_nocheck)

/* int userspace_2from_nocheck(void *address, uint16_t *store) */ 
BEGIN_FUNCTION(userspace_2from_nocheck)           
	pushq %rbp
	mov %rsp, %rbp
1:	movzwq (%rdi), %rax
	movw %ax, (%rsi)
	movabsq $StatusOk, %rax
2:      pop %rbp
	ret
3:      mov $StatusPageFault, %rax
	jmp 2b
.section pagefault_fixups, "a"
	.quad 1b, 3b
.previous        
END_OBJECT(userspace_2from_nocheck)
		
/* int userspace_1from_nocheck(void *address, uchar *store) */ 
BEGIN_FUNCTION(userspace_1from_nocheck)           
	pushq %rbp
	mov %rsp, %rbp
1:	movzbq (%rdi), %rax
	movb %al, (%rsi)
	movabsq $StatusOk, %rax
2:      pop %rbp
	ret
3:      mov $StatusPageFault, %rax
	jmp 2b
.section pagefault_fixups, "a"
	.quad 1b, 3b
.previous        
END_OBJECT(userspace_1from_nocheck)

/* int userspace_8to_nocheck(void *address, uint32_t store) */
BEGIN_FUNCTION(userspace_8to_nocheck)
	pushq %rbp
	mov %rsp, %rbp
1:	movq %rsi, (%rdi)
	movabsq $StatusOk, %rax
2:      pop %rbp
	ret     
3:      mov $StatusPageFault, %rax
	jmp 2b
.section pagefault_fixups, "a"
	.quad 1b, 3b
.previous
END_OBJECT(userspace_8to_nocheck)

/* int userspace_4to_nocheck(void *address, uint32_t store) */
BEGIN_FUNCTION(userspace_4to_nocheck)
	pushq %rbp
	mov %rsp, %rbp
1:	movl %esi, (%rdi)
	movabsq $StatusOk, %rax
2:      pop %rbp
	ret     
3:      mov $StatusPageFault, %rax
	jmp 2b
.section pagefault_fixups, "a"
	.quad 1b, 3b
.previous
END_OBJECT(userspace_4to_nocheck)        

/* int userspace_2to_nocheck(void *address, uint16_t store) */
BEGIN_FUNCTION(userspace_2to_nocheck)
	pushq %rbp
	mov %rsp, %rbp
	mov %rsi, %rax
1:	movw %ax, (%rdi)
	movabsq $StatusOk, %rax
2:      pop %rbp
	ret
3:      mov $StatusPageFault, %rax
	jmp 2b
.section pagefault_fixups, "a"
	.quad 1b, 3b
.previous        
END_OBJECT(userspace_2to_nocheck)
		
/* int userspace_1to_nocheck(void *address, uchar store) */
BEGIN_FUNCTION(userspace_1to_nocheck)
	pushq %rbp
	mov %rsp, %rbp
	mov %rsi, %rax
1:	movb %al, (%rdi)
	movabsq $StatusOk, %rax
2:      pop %rbp
	ret
3:      mov $StatusPageFault, %rax
	jmp 2b
.section pagefault_fixups, "a"
	.quad 1b, 3b
.previous
END_OBJECT(userspace_1to_nocheck)        

/* int userspace_memcpy_nocheck(void *dest, void *source, unsigned int length) */
/* No, this isn't the Fastest MemCopy In The West... */
BEGIN_FUNCTION(userspace_memcpy_nocheck)
	pushq %rbp
	mov %rsp, %rbp
	mov %rdx, %rcx
	cld
1:      rep movsb
	movabsq $StatusOk, %rax
2:      pop %rbp
	ret
3:      mov $StatusPageFault, %rax
	jmp 2b
.section pagefault_fixups, "a"
	.quad 1b, 3b
.previous
END_OBJECT(userspace_memcpy_nocheck)
