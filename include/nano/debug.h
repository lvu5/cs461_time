//______________________________________________________________________________
/// Debug macros
//
// Mar-2008: Andrei Warkentin
//______________________________________________________________________________

#ifndef __DEBUG_H__ // vs. common/debug.h.
#define __DEBUG_H__

#include <ethos/assert.h>
#include <nano/elf.h>
#include <nano/cpuPrivileged.h>

// Freezes the kernel on especially grievious bugs =).
void   debugExit(arch_interrupt_regs_t *regs);

// Initializes debugging support such as symbols, etc.
Status debugInit(void);

// Given a symbol name, returns the symbol structure.
//Status debugLookupName(const char *name, const arch_elf_sym_t **found_symbol);

// Given an address, figures out the name of the symbol containing this address 
// and the actual symbols address.
Status debugLookupAddress(vaddr_t address, char **name, vaddr_t *actual_location);

void   debugUnwind(vaddr_t frame_address, bool is_regs);
#endif
