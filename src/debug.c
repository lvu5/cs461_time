//______________________________________________________________________________
// Debugging and kernel bugchecking-related.
// Mar-2008: Andrei Warkentin
// Jul-2010: Patrick Gavlin
//______________________________________________________________________________

#include <nano/common.h>
#include <nano/initialStore.h>
#include <nano/elf.h>
#include <nano/x86_64/elf.h>
#include <nano/mm_public.h>
#include <nano/archDebug.h>
#include <nano/version.h>
#include <nano/xenEvent.h>
#include <nano/xenSchedule.h>
#include <nano/console.h>
#include <nano/schedPrivileged.h>


// Symtab/Strtab references.
static arch_elf_shdr_t *_debugSymtab = NULL;
static arch_elf_shdr_t *_debugStrtab = NULL;

extern char stack[];

// Unwind info
struct unwind_info {
    vaddr_t fp, pc;
};

// Where we look for symbols...
#define SYMBOLS_FILE "ethos.elf"

//______________________________________________________________________________
/// Given a symbol name, returns the symbol structure.
//______________________________________________________________________________
Status
debugLookupName(const char *name, const arch_elf_sym_t **found_symbol)
{
    ASSERT(name);
    ASSERT(found_symbol);
    const arch_elf_sym_t     *symbol = (const arch_elf_sym_t *) (_debugSymtab->va); 
    const arch_elf_sym_t *symtab_end = (const arch_elf_sym_t *) (_debugSymtab->va + _debugSymtab->size);
    Status status = StatusNotFound;
    while (symbol < symtab_end)
	{
	    if (unlikely(strcmp(name, (char*) (_debugStrtab->va + symbol->name)) == 0))
		{
		    *found_symbol = symbol;
		    status = StatusOk;
		    break;
		}
	    symbol++;      
	}  
    return status;
}

//______________________________________________________________________________
/// Given an address, figures out the name of the symbol containing this address 
/// and the actual symbols address.
//______________________________________________________________________________
Status
debugLookupAddress(vaddr_t address, char **name, vaddr_t *actual_location)
{
    Status status = StatusOk;
    ASSERT(name);
    ASSERT(actual_location);
    if (unlikely(!_debugSymtab || !_debugStrtab))
	{
	    status = StatusFail;
	    goto done;
	}
    const arch_elf_sym_t     *symbol = (const arch_elf_sym_t *) (_debugSymtab->va); 
    const arch_elf_sym_t *symtab_end = (const arch_elf_sym_t *) (_debugSymtab->va + _debugSymtab->size);
    status = StatusNotFound;
    while (symbol < symtab_end)
	{
	    if (unlikely(((address >symbol->value) && (address < symbol->value + symbol->size)) ||
			 (address == symbol->value)))
		{
		    *actual_location = symbol->value;
		    *name =  (char*) (_debugStrtab->va + symbol->name);
		    status = StatusOk;
		    break;
		}
	    symbol++;      
	}  
 done:
    return status;
}

//______________________________________________________________________________
/// Finds the symbol tables if present so we can have better debugging.
//______________________________________________________________________________
static
Status
_debugFindSymbols(void)
{
    arch_elf_half sh_index;
    vaddr_t elf_file = 0;
    unsigned long elf_file_length = 0;
    const arch_elf_sym_t *version_symbol = NULL;
    Status status = StatusOk;
  
    // Try locating the symbols file...
    Ref *ref=NULL;
    status = initialStoreFind(SYMBOLS_FILE, &ref);
    elf_file = (vaddr_t) ref->ptr;
    elf_file_length = refSize(ref);

    // Don't assert. It's not a mistake to not have symbols.
    if (status != StatusOk)
	{
	    goto done;
	}
  
    vaddr_t elf_file_end = (vaddr_t) elf_file + elf_file_length;
    arch_elf_ehdr_t *elf_header = (arch_elf_ehdr_t *) elf_file;
    if (
	elf_header->ident[ELF_EI_MAG0] != ELF_MAG0 ||
	elf_header->ident[ELF_EI_MAG1] != ELF_MAG1 ||
	elf_header->ident[ELF_EI_MAG2] != ELF_MAG2 ||
	elf_header->ident[ELF_EI_MAG3] != ELF_MAG3 ||
	elf_header->ident[ELF_EI_CLASS] != ARCH_ELF_CLASS ||
	elf_header->ident[ELF_EI_DATA] != ARCH_ELF_DATA ||
	elf_header->type != ELF_ET_EXEC ||
	elf_header->version != ELF_EV_CURRENT ||
	elf_header->machine != ARCH_ELF_MACHINE ||
	elf_header->eh_size != sizeof(arch_elf_ehdr_t) ||
	elf_header->sh_ent_size != sizeof(arch_elf_shdr_t) ||
	elf_header->sh_num == 0
	)
	{
	    // Not ELF or the right kind of elf.
	    status = StatusFail;
	    ASSERT_OK(status);
	    goto done;
	}
  
    // Get the section headers.
    arch_elf_shdr_t *sheader = (arch_elf_shdr_t *) ((uintptr_t) elf_file + elf_header->sh_off);
    arch_elf_shdr_t *sheader_end
	= (arch_elf_shdr_t *) ((uintptr_t) sheader + elf_header->sh_num * elf_header->sh_ent_size);

    // Out of bounds?
    if ((vaddr_t) sheader >= elf_file_end ||
	(vaddr_t) sheader_end > elf_file_end)
	{
	    status = StatusFail;
	    ASSERT_OK(status);
	}

    // Find the right sections.
    for (sh_index = 0; sh_index < elf_header->sh_num; sh_index++)
	{
	    if (sheader->sh_type == ELF_SHT_SYMTAB)
		{
		    _debugSymtab = sheader;

		    // File = Image alignment so whatever.
		    sheader->va = elf_file + sheader->file_offset;
		}
	    else if (sheader->sh_type == ELF_SHT_STRTAB && sh_index != elf_header->sh_str_index)
		{
		    _debugStrtab = sheader;

		    // File = Image alignment so whatever.
		    sheader->va = elf_file + sheader->file_offset;
		}
	    sheader = (arch_elf_shdr_t *) ((uintptr_t) sheader + elf_header->sh_ent_size);
	}

    // Good work. Now find the version symbol and ensure the version is the same.
    status = debugLookupName(SEXPAND(VERSION_SYMBOL), &version_symbol);
    if (status != StatusOk)
	{
	    goto done;
	}
    if (version_symbol->value != (arch_elf_addr) &VERSION_SYMBOL)
	{
	    status = StatusFail;
	    goto done;
	}
    if (version_symbol->shndx >= elf_header->sh_num)
	{
	    status = StatusFail;
	    goto done;
	}
    arch_elf_shdr_t *version_symbol_section = (arch_elf_shdr_t *) ((uintptr_t) elf_file + 
							     elf_header->sh_off + 
							     (elf_header->sh_ent_size * version_symbol->shndx));   
    if ((version_symbol->value - version_symbol_section->va) < 0)
	{
	    status = StatusFail;
	    goto done;
	}

    // Does version match?
    if (strncmp(VERSION_SYMBOL, 
		(char*) (
			 elf_file + 
			 version_symbol_section->file_offset + 
			 (version_symbol->value - version_symbol_section->va)
			 ), 
		strlen(VERSION_SYMBOL)) != 0)
	{
	    status = StatusFail;
	    goto done;
	}

 done:
    if (status != StatusOk)
	{
	    _debugSymtab = NULL;
	    _debugStrtab = NULL;
	}
    return status;
}

Status
debugInit(void)
{
    Status status = _debugFindSymbols();
    if (status != StatusOk)
	{
	    xprintLog("No symbols or out-of-date symbols\n");
	    status = StatusOk;
	}
    return status;
}

//
// Prints the symbol name during unwind.
//
static
void 
_debugUnwindPrint(vaddr_t sym)
{
    char *sym_name;
    vaddr_t actual;

    printfLog("[0x%p]", sym);
    if (debugLookupAddress(sym, &sym_name, &actual) == StatusOk) 
	{
	    printfLog(" %s + %x", sym_name, sym - actual);
	}
    printfLog("\n");
}


//
// Returns true if the unwind procedure should continue.
//
static
bool
_debugUnwindValid(vaddr_t return_address, 
		 ArchPrivilegeState state,
		 vaddr_t frame_address)
{
    vaddr_t stack_base;
    vaddr_t stack_end;
    vaddr_t pc_base;
  
    ASSERT(PRIVILEGED_MODE == state);

    pc_base = (vaddr_t) VIRT_START;

    // For the freak case of a panic while setting up the first process...
    stack_base = (vaddr_t) stack;
    stack_end = stack_base + KERNEL_STACK_SIZE;

    return (return_address > pc_base &&
	    frame_address >= stack_base && 
	    frame_address < stack_end);
}

//
// Safe-reader helper. The unwinder could be used in an interrupt
// context and we don't want to cause another exception.
//
static
Status
_debugUnwindCheckRead(vaddr_t addr, vaddr_t *vaddr)
{
    Status status = vaddrIsMapped(addr);
    if (status != StatusNotFound)
	{
	    *vaddr = *(vaddr_t *) addr;
	}
    return status;
}

//
// Stack unwinder. All it needs is either a frame to unwind from,
// or an interrupt context structure (for the later case, is_regs is true).
//
void
debugUnwind(vaddr_t frame_address, bool is_regs)
{
    vaddr_t ra_address;
    vaddr_t fa_address;
  
    union
    {
	vaddr_t address;
	arch_reverse_frame_t *revf;
	arch_interrupt_regs_t *regs;
    } frame;

    vaddr_t return_address;
    ArchPrivilegeState state = PRIVILEGED_MODE;
    frame.address = frame_address;

    printfLog("Backtrace: \n");
    do 
	{

	    //
	    // Read the next return address.
	    //

	    if (is_regs)
		{
		    ra_address = (vaddr_t) &ArchRegsIp(frame.regs);
		}
	    else
		{
		    ra_address = (vaddr_t) &ARCH_REVF_RETURN_ADDRESS(frame.revf);
		}

	    if (_debugUnwindCheckRead(ra_address, &return_address) == StatusNotFound)
		{
		    printfLog("Could not read return address at 0x%x\n", ra_address);
		    break;
		}

	    //
	    // Read the next frame pointer.
	    //

	    if (is_regs)
		{
		    is_regs = false;
		    fa_address = (vaddr_t) &ArchRegsFp(frame.regs);

		    //
		    // Need to figure out what state the next frame should be treated as,
		    // as we might have kernel or user stack after this.
		    // I suppose if we read ra_address this should never step on a page_fault...
		    //

		    state = archPrivilegeState(frame.regs);
		    printfLog("-----> Next frame returning from interrupt context is %s space <-----\n",
				state != PRIVILEGED_MODE ? "user" : "kernel");
		}
	    else
		{
		    if (archDebugUnwindNextFrameIsRegs(return_address))
			{
			    is_regs = true;

			    //
			    // "Frame" is the regs structure, which is passed by entryException
			    // and the like as a parameter on the stack to the next called
			    // function.
			    //

			    fa_address = (vaddr_t) &ARCH_REVF_REGS_FRAME(frame.revf);
			}
		    else
			{
			    fa_address = (vaddr_t) &ARCH_REVF_OLD_FRAME(frame.revf);
			}
		}

	    if (_debugUnwindCheckRead(fa_address, &frame.address) == StatusNotFound)
		{
		    printfLog("Could not read frame address at 0x%x\n", ra_address);
		    break;
		}

	    _debugUnwindPrint(return_address);
	} while(_debugUnwindValid(return_address, state, frame.address));
}

//______________________________________________________________________________
// Catastrophic failure.  Log error, registers, and shutdown ethos.
//______________________________________________________________________________
void
debugExit(arch_interrupt_regs_t *regs)
{

    __cli();
    xprintLog("Terminating Ethos Kernel\n");

    // We don't want to cause any more exceptions, so
    // treat everything as if it could.
    if (regs && vaddrRangeIsMapped((vaddr_t) regs, sizeof(arch_interrupt_regs_t)) == StatusOk)
	{
	    archDebugRegsDump(regs);
	    debugUnwind((vaddr_t) regs, true);
	}
    else
	{
	    debugUnwind((vaddr_t) __builtin_frame_address(0), false);
	}

    // Gack... if we leave before the xencons ring buffer has been flushed,
    // we have no idea what happened... so spin while we wait for it to
    // be flushed.
    consoleFlush();
    for (;;)
	{
	    xenScheduleShutdown(SHUTDOWN_crash);
	}
}
