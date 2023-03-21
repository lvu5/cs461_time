// x86_32-specific ELF bits.
// Mar-2008: Andrei Warkentin

#ifndef x86_64_ELF
#define x86_64_ELF

#define ARCH_ELF_CLASS   (ELF_CLASS_64)
#define ARCH_ELF_DATA    (ELF_DATA_2LSB)
#define ARCH_ELF_MACHINE (62) /* EM_X86_64 */

typedef elf64_addr  arch_elf_addr;
typedef elf64_half  arch_elf_half;
typedef elf64_off   arch_elf_off;
typedef elf64_sword arch_elf_sword;
typedef elf64_word  arch_elf_word;

typedef elf64_ehdr_t arch_elf_ehdr_t;
typedef elf64_shdr_t arch_elf_shdr_t;
typedef elf64_sym_t  arch_elf_sym_t;
typedef elf64_rel_t  arch_elf_rel_t;
typedef elf64_rela_t arch_elf_rela_t;
typedef elf64_phdr_t arch_elf_phdr_t;

#endif
