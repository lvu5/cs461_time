//______________________________________________________________________________
// Architecture-specific debug code.
// Mar-2008: Andrei Warkentin
//______________________________________________________________________________

#ifndef _ARCH_DEBUG_H_
#define _ARCH_DEBUG_H_

#include <nano/cpuPrivileged.h>

// Dumps the contents of the regs structure to the screen.
void archDebugRegsDump(arch_interrupt_regs_t *regs);

void archDebugRegsDump2(arch_interrupt_regs_t *regs);


//
// Returns true if the frame for function corresponding to
// return_address is actually an interrupt context struture.
//
bool archDebugUnwindNextFrameIsRegs(vaddr_t returnAddress);

#endif
