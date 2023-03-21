#ifndef CPU_H
#define CPU_H

#if defined(__x86_32__)
#include <nano/x86_32/cpu.h>
#elif defined(__x86_64__)
#include <nano/x86_64/cpu.h>
#else
#error Unsupported architecture.
#endif

#ifndef __ASSEMBLY__

// Defined in archProcess.c

#endif
#endif
