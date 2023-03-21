// CPU odds and ends.
// Mar-2008: Andrei Warkentin

#ifndef x86_64_CPU_H
#define x86_64_CPU_H

#ifndef __ASSEMBLY__

// Must use FXSAVE64/FXRSTOR64 for the below to be valid
typedef struct arch_fpu_state_s
{
	uint16_t x87_control;
	uint16_t x87_status;
	uint8_t  x87_tag;
	uint8_t  rsv0;
	uint16_t x87_fop;
	uint64_t x87_ip;
	uint64_t x87_dp;
	uint32_t sse_mxcsr;
	uint32_t sse_mxcsr_mask;
	uint64_t mm[16];
	uint64_t xmm[16];
	uint64_t pad[28];		// Pads the struct out to 512 bytes
} __attribute__ ((__packed__, aligned(16))) arch_fpu_state_t;


// Processor state saved by arch_switch_context 
// (see $(E)/arch/x86/x86_64/entry.S)
typedef struct arch_switch_context_regs_s
{
	unsigned long rflags;
	unsigned long r15;
	unsigned long r14;
	unsigned long r13;
	unsigned long r12;
	unsigned long rbx;
	unsigned long rbp;
} arch_switch_context_regs_t;

#endif
#endif
