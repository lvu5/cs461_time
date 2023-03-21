// Synchronization-related.

#ifndef x86_64_SYNCHRO_H
#define x86_64_SYNCHRO_H

// Needs to be not this for SMP.
#define LOCK_PREFIX ""
#define LOCK

// This is a barrier for the compiler only, NOT the processor!
#define barrier() __asm__ __volatile__("": : :"memory")

// Processor barriers.
#define mb()    __asm__ __volatile__ ("mfence":::"memory")
#define rmb()   __asm__ __volatile__ ("lfence":::"memory")
#define wmb()	__asm__ __volatile__ ("sfence" ::: "memory") /* From CONFIG_UNORDERED_IO (linux) */

#endif 
