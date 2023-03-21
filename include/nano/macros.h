//______________________________________________________________________________
// Useful macros.
// Mar-2008: Andrei Warkentin
//______________________________________________________________________________

#ifndef __MACROS_H__
#define __MACROS_H__

#ifdef __ASSEMBLY__

// This lets us properly account for syms defined in asm.
#define BEGIN_FUNCTION(X) \
.globl X ;\
.type X, @function ;\
X:

#define BEGIN_LOCAL_FUNCTION(X) \
.type X, @function ;\
X:

#define BEGIN_OBJECT(X) \
.globl X ;\
.type X, @object ;\
X:

#define BEGIN_LOCAL_OBJECT(X) \
.type X, @object ;\
X:

#define END_OBJECT(X) \
.size X, .-X

#else

// Rounds down to the nearest "on" size.
#define ROUND_DOWN_ON(x, on) ((x) & ~((on) - 1))

// Rounds  up to the nearest "on" size.
#define ROUND_UP_ON(x, on) ROUND_DOWN_ON((x) + (on) - 1, (on))

// Used by the SEXPAND macro.
#define SEXPAND_PRE(WHAT) #WHAT

// Allows macro expansion in places where 
// the final result should be embedded in a
// string, like inline assembly.
#define SEXPAND(WHAT) SEXPAND_PRE(WHAT)

// Make sure code is emitted even if symbol is undereferenced.
#define USED __attribute__ ((used))

// Branch prediction, verification.
#if __GNUC__ == 2 && __GNUC_MINOR__ < 96
#define __builtin_expect(x, expected_value) (x)
#endif
#define unlikely(x)  __builtin_expect((x), 0)
#define likely(x) __builtin_expect(!!(x), 1)

// Doing min/max like this let's stuff 
// like min(a++, b++) not break.
#define MIN(a,b) ({ typeof(a) _a = (a); typeof(b) _b = (b); _a < _b ? _a : _b; })
#define MAX(a,b) ({ typeof(a) _a = (a); typeof(b) _b = (b); _a > _b ? _a : _b; })

// Bits to byte conversion.
#define BITS_TO_B(x) ((x) >> 3)

// Byte to Kilobyte conversion.
#define B_TO_KB(x) ((x) >> 10)

// Kilobyte to Megabyte conversion.
#define KB_TO_MB(x) B_TO_KB(x)

// Megabyte to Gigabyte conversion.
#define MB_TO_GB(x) B_TO_KB(x)

// Byte to Megabyte conversion.
#define B_TO_MB(x) KB_TO_MB(B_TO_KB(x))

// Byte to bits conversion.
#define B_TO_BITS(x) ((x) << 3)

// Kilobyte to Byte conversion.
#define KB_TO_B(x) ((x) << 10)

// Gigabyte to Megabyte conversion.
#define GB_TO_MB(x) KB_TO_B(x)

// Megabyte to Kilobyte conversion.
#define MB_TO_KB(x) KB_TO_B(x)

// Megabyte to Byte conversion.
#define MB_TO_B(x) KB_TO_B(MB_TO_KB(x))

// Gigabyte to Byte conversion.
#define GB_TO_B(x) MB_TO_B(GB_TO_MB(x))

// Get the size of an array.
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#endif
#endif
