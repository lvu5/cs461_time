#ifndef HYPERCALL_H
#define HYPERCALL_H

#if defined(__x86_32__)
#include <nano/x86_32/hypercall.h>
#elif defined(__x86_64__)
#include <nano/x86_64/hypercall.h>
#else
#error Unsupported architecture.
#endif

#endif
