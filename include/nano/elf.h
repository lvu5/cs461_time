//______________________________________________________________________________
// Executable and Linking Format - related.
// Mar-2008: Andrei Warkentin
//______________________________________________________________________________

#ifndef __ELF_H__
#define __ELF_H__

#include <nano/intTypes.h>
#include <nano/ethosTypes.h>
#include <nano/ref.h>

// ELF32 types.
typedef uint32_t elf32_addr;
typedef uint16_t elf32_half;
typedef uint32_t elf32_off;
typedef int32_t  elf32_sword;
typedef uint32_t elf32_word;

// ELF64 types.
typedef uint64_t elf64_addr;
typedef uint64_t elf64_off;
typedef uint16_t elf64_half;
typedef uint32_t elf64_word;
typedef int32_t  elf64_sword;
typedef uint64_t elf64_xword;
typedef int64_t  elf64_sxword;

// e_ident indices.
// Identification.
#define ELF_EI_MAG0     (0)
#define ELF_EI_MAG1     (1)
#define ELF_EI_MAG2     (2)
#define ELF_EI_MAG3     (3)

// File class.
#define ELF_EI_CLASS    (4)

// Data encoding.
#define ELF_EI_DATA     (5)

// File version.
#define ELF_EI_VERSION  (6)

// Size of padding bytes.
#define ELF_EI_PAD      (7)

// Size of e_ident[]
#define ELF_EI_NIDENT   (16)

// e_type values.
// No file type.
#define ELF_ET_NONE     (0)

// Relocatable.
#define ELF_ET_REL      (1)

// Executable.
#define ELF_ET_EXEC     (2)

// SO.
#define ELF_ET_DYN      (3)

 // Core file.
#define ELF_ET_CORE     (4)

// Processor-specific.
#define ELF_ET_LOPROC   (0xff00)
#define ELF_ET_HIPROC   (0xffff)

// e_machine types.
// No machine.
#define ELF_EM_NONE     (0)

// e_version values.
// Invalid version.
#define ELF_EV_NONE     (0)

// Current version.
#define ELF_EV_CURRENT  (1)

// Values for e_ident parts.
#define ELF_MAG0        (0x7f)
#define ELF_MAG1        ('E')
#define ELF_MAG2        ('L')
#define ELF_MAG3        ('F')

// Invalid class.
#define ELF_CLASS_NONE   (0)

// 32-bit.
#define ELF_CLASS_32     (1)

// 64-bit.
#define ELF_CLASS_64     (2)

// Invalid data encoding.
#define ELF_DATA_NONE    (0)

// Little-endian.
#define ELF_DATA_2LSB    (1)

// Big-endian.
#define ELF_DATA_2MSB    (2)

// As per spec.
#define ELF_VERSION      (ELF_EV_CURRENT)

// ELF32 header.
typedef struct elf32_ehdr_s
{
    uchar ident[ELF_EI_NIDENT];     // ELF indentification.
    elf32_half   type;                // Object file type.
    elf32_half   machine;             // Machine type.
    elf32_word   version;             // Object file version.
    elf32_addr   entry;               // Entry VA.
    elf32_off    ph_off;              // ELF PH offset.
    elf32_off    sh_off;              // ELF SH offset.
    elf32_word   flags;               // Processor-specific flags.
    elf32_half   eh_size;             // Size of ELF header in bytes.
    elf32_half   ph_ent_size;         // PH entry size in bytes.
    elf32_half   ph_num;              // Number of PH entries.
    elf32_half   sh_ent_size;         // SH entry size in bytes.
    elf32_half   sh_num;              // Number of SH entries.
    elf32_half   sh_str_index;        // SH table index for section string table.
} elf32_ehdr_t;

// ELF64 header.
typedef struct elf64_ehdr_s
{
    uchar ident[ELF_EI_NIDENT];     // ELF indentification.
    elf64_half   type;                // Object file type.
    elf64_half   machine;             // Machine type.
    elf64_word   version;             // Object file version.
    elf64_addr   entry;               // Entry VA.
    elf64_off    ph_off;              // ELF PH offset.
    elf64_off    sh_off;              // ELF SH offset.
    elf64_word   flags;               // Processor-specific flags.
    elf64_half   eh_size;             // Size of ELF header in bytes.
    elf64_half   ph_ent_size;         // PH entry size in bytes.
    elf64_half   ph_num;              // Number of PH entries.
    elf64_half   sh_ent_size;         // SH entry size in bytes.
    elf64_half   sh_num;              // Number of SH entries.
    elf64_half   sh_str_index;        // SH table index for section string table.
} elf64_ehdr_t;


// Special section indexes.
// Meaningless section reference.
#define ELF_SHN_UNDEF           0       

// Lower bound of reserved indices.
#define ELF_SHN_LORESERVE       0xff00  

// Processor-specific semantics.
#define ELF_SHN_LOPROC          0xff00
#define ELF_SHN_HIPROC          0xff1f

// Absolute values for corresponding reference.
#define ELF_SHN_ABS             0xfff1

// Common symbols.
#define ELF_SHN_COMMON          0xfff2

// Upper bound of reserved indices. 
#define ELF_SHN_HIRESERVE       0xffff

// Section headers.
typedef struct elf32_shdr_s
{
    elf32_word   name;        // Name of section. This is an index into the string table.
    elf32_word   sh_type;     // Section type of contents and semantics.
    elf32_word   flags;       // Miscellaneous attributes.
    elf32_addr   va;          // Section VA. 
    elf32_off    file_offset; // Byte offset of section from beginning of file.
    elf32_word   size;        // Size in bytes.
    elf32_word   table_link;  // Section table header link.
    elf32_word   extra_info;  // Extra info, depends on type.
    elf32_word   va_align;    // Value of Va must be congruent to 0 mod VaAlign.
    elf32_word   ent_size;    // Size of entries if section contains fixed-size entries.
} elf32_shdr_t;

typedef struct elf64_shdr_s
{
    elf64_word   name;        // Name of section. This is an index into the string table.
    elf64_word   sh_type;     // Section type of contents and semantics.
    elf64_xword  flags;       // Miscellaneous attributes.
    elf64_addr   va;          // Section VA. 
    elf64_off    file_offset; // Byte offset of section from beginning of file.
    elf64_xword  size;        // Size in bytes.
    elf64_word   table_link;  // Section table header link.
    elf64_word   extra_info;  // Extra info, depends on type.
    elf64_xword  va_align;    // Value of Va must be congruent to 0 mod VaAlign.
    elf64_xword  ent_size;    // Size of entries if section contains fixed-size entries.
} elf64_shdr_t;


// Section types for sh_type.
// Inactive section header.
#define ELF_SHT_NULL            0

// Program-defined contents.
#define ELF_SHT_PROGBITS        1

// Symbol table.
#define ELF_SHT_SYMTAB          2

// String table.
#define ELF_SHT_STRTAB          3

// Relocation entries with explicit addends.
#define ELF_SHT_RELA            4

// Symbol hash table for dynamic linking.
#define ELF_SHT_HASH            5

// Information for dynamic linking.
#define ELF_SHT_DYNAMIC         6

// Note section.
#define ELF_SHT_NOTE            7

// Like progbits, but occupies no space.
#define ELF_SHT_NOBITS          8

// Relocation entries without explicit addends.
#define ELF_SHT_REL             9

// Reserved but unsupported.
#define ELF_SHT_SHLIB           10

// Alternative/additional symbol table for dynamic linking.
#define ELF_SHT_DYNSYM          11

// Processor-specific semantics.
#define ELF_SHT_LOPROC          0x70000000
#define ELF_SHT_HIPROC          0x7fffffff

// Environment-specific.
#define ELF_SHT_LOOS            0x60000000
#define ELF_SHT_HIOS            0x6fffffff   

// Application-specific.
#define ELF_SHT_LOUSER          0x80000000
#define ELF_SHT_HIUSER          0xffffffff

// Values for sh_flags.
// Sections contains data that should be writable during execution.
#define ELF_SHF_WRITE           0x1

// Section occupies memory during execution.
#define ELF_SHF_ALLOC           0x2

// Section contains executable machine instructions.
#define ELF_SHF_EXECINSTR       0x4

// All bits in this mask are reserved for os-specific semantics.
#define ELF_SHF_MASKOS          0x0f000000

// All bits in this mask are reserved for processor-specific semantics.
#define ELF_SHF_MASKPROC        0xf0000000

// The zero index into the symbol table both designates the first (reserved) entry
// in the table and serves as the undefined symbol index.
#define ELF_STN_UNDEF           0

// Symbol table entries.
typedef struct elf32_sym_s
{
    elf32_word   name;        // Index into string table.
    elf32_addr   value;       // Value of the associated symbol.
    elf32_word   size;        // Size associated with symbol.
    uchar      info;        // Type and binding attributes.
    uchar      other;       // No defined meaning.
    elf32_half   shndx;       // Section header table index.
} elf32_sym_t;

typedef struct elf64_sym_s
{
    elf64_word   name;        // Index into string table.
    uchar      info;        // Type and binding attributes.
    uchar      other;       // No defined meaning.
    elf64_half   shndx;       // Section header table index.
    elf64_addr   value;       // Value of the associated symbol.
    elf64_xword  size;        // Size associated with symbol.
} elf64_sym_t;

// Manipulating the info field in the symbol table entry.
#define ELF32_ST_BIND(info)       ((info) >> 4)
#define ELF32_ST_TYPE(info)       ((info) & 0xf)
#define ELF32_ST_INFO(bind, type) (((bind) << 4) + ((type) & 0xf))

// Symbol binding.
// Local symbols.
#define ELF_STB_LOCAL   0

// Global symbols.
#define ELF_STB_GLOBAL  1

// Weak symbols.
#define ELF_STB_WEAK    2

// Processor specific semantics.
#define ELF_STB_LOPROC  13
#define ELF_STB_HIPROC  15

// Symbol types.
// Not specified.
#define ELF_STT_NOTYPE  0

// Data object such as a variable or array.
#define ELF_STT_OBJECT  1

// Function or other executable code.
#define ELF_STT_FUNC    2

// Section (primarily used for relocation).
#define ELF_STT_SECTION 3

// Gives the name of the source file associated with object.
#define ELF_STT_FILE    4

// Environment-specific.
#define ELF_STT_LOOS    10
#define ELF_STT_HIOS    12

// Processor-specific semantics.
#define ELF_STT_LOPROC  13
#define ELF_STT_HIPROC  15

// Relocation entry without explicit addends.
typedef struct elf32_rel_s
{
    elf32_addr offset; // Location where to apply the relocation.
    elf32_word info;   // Symbol table index + relocation type.
} elf32_rel_t;

// Relocation entry with explicit addends.
typedef struct elf32_rela_s
{
    elf32_addr  offset; // Location where to apply the relocation.
    elf32_word  info;   // Symbol table index + relocation type.
    elf32_sword addend; // Constant added used to compute relocated value.
} elf32_rela_t;

// Relocation entry without explicit addends.
typedef struct elf64_rel_s
{
    elf64_addr  offset; // Location where to apply the relocation.
    elf64_xword info;   // Symbol table index + relocation type.
} elf64_rel_t;

// Relocation entry with explicit addends.
typedef struct elf64_rela_s
{
    elf64_addr   offset; // Location where to apply the relocation.
    elf64_xword  info;   // Symbol table index + relocation type.
    elf64_sxword addend; // Constant added used to compute relocated value.
} elf64_rela_t;


// Computing symbol index and relocation type from Info.
#define ELF32_R_SYM(info)       ((info) >> 8)
#define ELF32_R_TYPE(info)      ((uchar)(info))
#define ELF32_R_INFO(sym, type) (((sym) << 8) + (uchar)(type))

#define ELF64_R_SYM(info)       ((info) >> 32)
#define ELF64_R_TYPE(info)      ((info) & 0xffffffffL)
#define ELF64_R_INFO(sym, type) (((sym) << 32) + ((t) & 0xffffffffL))

// Program headers.
typedef struct elf32_phdr_s
{
    elf32_word   type;         // Segment kind. 
    elf32_off    offset;       // Offset of segment in bytes from file beginning.
    elf32_addr   vaddr;        // VA of segment.
    elf32_addr   paddr;        // PA of segment if relevant.
    elf32_word   file_sz;      // Bytes in the file image of segment.
    elf32_word   mem_sz;       // Bytes in memory image of segment.
    elf32_word   flags;        // Segment flags.
    elf32_word   align;        // File and memory alignment.
} elf32_phdr_t;

// Program header.
typedef struct elf64_phdr_s
{
    elf64_word   type;         // Segment kind. 
    elf64_word   flags;        // Segment flags.
    elf64_off    offset;       // Offset of segment in bytes from file beginning.
    elf64_addr   vaddr;        // VA of segment.
    elf64_addr   paddr;        // PA of segment if relevant.
    elf64_xword  file_sz;      // Bytes in the file image of segment.
    elf64_xword  mem_sz;       // Bytes in memory image of segment.
    elf64_xword  align;        // File and memory alignment.
} elf64_phdr_t;


// Segment types.
// Ignored entry.
#define ELF_PT_NULL     0

// Loadable segment.
#define ELF_PT_LOAD     1

// Dynamic linking information.
#define ELF_PT_DYNAMIC  2

// Interpreter information.
#define ELF_PT_INTERP   3

// Auxiliary information.
#define ELF_PT_NOTE     4

// Reserved and unsupported.
#define ELF_PT_SHLIB    5

// Program header table itself (valid only in memory image, if present).
#define ELF_PT_PHDR     6

// Segment is eXecutable.
#define PF_X        (0x1)

// Segment is writable.
#define PF_W        (0x2)

// Segment is readable
#define PF_R        (0x4)

// Unspecified.
#define ELF64_PF_MASKPROC (0xff000000)
#define ELF64_PF_MASKOS   (0x00ff0000)

// Unspecified.
#define ELF32_PF_MASKPROC (0xf0000000)

// Environment specific semantics.
#define ELF_PT_LOOS     0x60000000
#define ELF_PT_HIOS     0x6fffffff

// Processor specific semantics.
#define ELF_PT_LOPROC   0x70000000
#define ELF_PT_HIPROC   0x7fffffff

typedef struct ProcessMemory ProcessMemory;

// Loads an ELF blob as part of creating process.
Status elfLoad(ProcessMemory *processMemory, vaddr_t *entry, Ref  *elfLoad);

#endif
