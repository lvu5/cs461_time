#ifndef BITS_H
#define BITS_H

#if defined(__x86_32__)
#include <ethos/kernel/x86_32/bits.h>
#elif defined(__x86_64__)
#include <ethos/kernel/x86_64/bits.h>
#else
#error Unsupported architecture.
#endif

#endif
