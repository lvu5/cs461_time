#ifndef SYNCHRO_H
#define SYNCHRO_H

#if defined(__x86_32__)
#include <ethos/kernel/x86_32/synchro.h>
#elif defined(__x86_64__)
#include <ethos/kernel/x86_64/synchro.h>
#else
#error Unsupported architecture.
#endif

// For public/io/ring.h macros.
#if __XEN_INTERFACE_VERSION__ >= 0x00030208
#define xen_mb()  mb()
#define xen_rmb() rmb()
#define xen_wmb() wmb()
#endif  // __XEN_INTERFACE_VERSION__


#endif 
