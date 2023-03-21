// Bit operations.

#ifndef x86_64_BITS_H
#define x86_64_BITS_H

#include <ethos/kernel/x86_64/synchro.h>

/**
 * test_and_clear_bit - Clear a bit and return its old value
 * @nr: Bit to clear
 * @addr: Address to count from
 *
 * This operation is atomic and cannot be reordered.  
 * It also implies a memory barrier.
 */
static __inline__ int test_and_clear_bit(int nr, volatile void * addr)
{
	int oldbit;

	__asm__ __volatile__( LOCK_PREFIX
		"btrl %2,%1\n\tsbbl %0,%0"
		:"=r" (oldbit),"=m" ((*(volatile long *) addr))
		:"dIr" (nr) : "memory");
	return oldbit;
}

static __inline__ int constant_test_bit(int nr, const volatile void * addr)
{
	return ((1UL << (nr & 31)) & (((const volatile unsigned int *) addr)[nr >> 5])) != 0;
}

static __inline__ int variable_test_bit(int nr, volatile const void * addr)
{
	int oldbit;

	__asm__ __volatile__(
		"btl %2,%1\n\tsbbl %0,%0"
		:"=r" (oldbit)
		:"m" ((*(volatile long *) addr)),"dIr" (nr));
	return oldbit;
}

#define test_bit(nr,addr) \
(__builtin_constant_p(nr) ? \
 constant_test_bit((nr),(addr)) : \
 variable_test_bit((nr),(addr)))


/**
 * set_bit - Atomically set a bit in memory
 * @nr: the bit to set
 * @addr: the address to start counting from
 *
 * This function is atomic and may not be reordered.  See __set_bit()
 * if you do not require the atomic guarantees.
 * Note that @nr may be almost arbitrarily large; this function is not
 * restricted to acting on a single-word quantity.
 */
static __inline__ void set_bit(int nr, volatile void * addr)
{
	__asm__ __volatile__( LOCK_PREFIX
		"btsl %1,%0"
		:"=m" ((*(volatile long *) addr))
		:"dIr" (nr) : "memory");
}

/**
 * clear_bit - Clears a bit in memory
 * @nr: Bit to clear
 * @addr: Address to start counting from
 *
 * clear_bit() is atomic and may not be reordered.  However, it does
 * not contain a memory barrier, so if it is used for locking purposes,
 * you should call smp_mb__before_clear_bit() and/or smp_mb__after_clear_bit()
 * in order to ensure changes are visible on other processors.
 */
static __inline__ void clear_bit(int nr, volatile void * addr)
{
	__asm__ __volatile__( LOCK_PREFIX
		"btrl %1,%0"
		:"=m" ((*(volatile long *) addr))
		:"dIr" (nr));
}

/**
 * __ffs - find first bit in word.
 * @word: The word to search
 *
 * Undefined if no bit exists, so code should check against 0 first.
 */
static __inline__ unsigned long __ffs(unsigned long word)
{
	__asm__("bsfq %1,%0"
		:"=r" (word)
		:"rm" (word));
	return word;
}

struct __synch_xchg_dummy { unsigned long a[100]; };
#define __synch_xg(x) ((struct __synch_xchg_dummy *)(x))
#define xchg(ptr,v) ((__typeof__(*(ptr)))__xchg((unsigned long)(v),(ptr),sizeof(*(ptr))))
struct __xchg_dummy { unsigned long a[100]; };
#define __xg(x) ((struct __xchg_dummy *)(x))

static inline unsigned long __xchg(unsigned long x, volatile void * ptr, int size)
{
	switch (size) {
		case 1:
			__asm__ __volatile__("xchgb %b0,%1"
				:"=q" (x)
				:"m" (*__xg(ptr)), "0" (x)
				:"memory");
			break;
		case 2:
			__asm__ __volatile__("xchgw %w0,%1"
				:"=r" (x)
				:"m" (*__xg(ptr)), "0" (x)
				:"memory");
			break;
		case 4:
			__asm__ __volatile__("xchgl %k0,%1"
				:"=r" (x)
				:"m" (*__xg(ptr)), "0" (x)
				:"memory");
			break;
		case 8:
			__asm__ __volatile__("xchgq %0,%1"
				:"=r" (x)
				:"m" (*__xg(ptr)), "0" (x)
				:"memory");
			break;
	}
	return x;
}

#define synch_cmpxchg(ptr, old, new) \
((__typeof__(*(ptr)))__synch_cmpxchg((ptr),\
                                     (unsigned long)(old), \
                                     (unsigned long)(new), \
                                     sizeof(*(ptr))))

static inline unsigned long __synch_cmpxchg(volatile void *ptr,
        unsigned long old,
        unsigned long new, int size)
{
    unsigned long prev;
    switch (size) {
        case 1:
            __asm__ __volatile__("lock; cmpxchgb %b1,%2"
                    : "=a"(prev)
                    : "q"(new), "m"(*__synch_xg(ptr)),
                    "0"(old)
                    : "memory");
            return prev;
        case 2:
            __asm__ __volatile__("lock; cmpxchgw %w1,%2"
                    : "=a"(prev)
                    : "r"(new), "m"(*__synch_xg(ptr)),
                    "0"(old)
                    : "memory");
            return prev;
        case 4:
            __asm__ __volatile__("lock; cmpxchgl %k1,%2"
                    : "=a"(prev)
                    : "r"(new), "m"(*__synch_xg(ptr)),
                    "0"(old)
                    : "memory");
            return prev;
        case 8:
            __asm__ __volatile__("lock; cmpxchgq %1,%2"
                    : "=a"(prev)
                    : "r"(new), "m"(*__synch_xg(ptr)),
                    "0"(old)
                    : "memory");
            return prev;
    }
    return old;
}


static __inline__ void synch_set_bit(int nr, volatile void * addr)
{
    __asm__ __volatile__ ( 
        "lock btsl %1,%0"
        : "=m" ((*(volatile long *) addr)) : "Ir" (nr) : "memory" );
}

static __inline__ void synch_clear_bit(int nr, volatile void * addr)
{
    __asm__ __volatile__ (
        "lock btrl %1,%0"
        : "=m" ((*(volatile long *) addr)) : "Ir" (nr) : "memory" );
}

static __inline__ int synch_test_and_set_bit(int nr, volatile void * addr)
{
    int oldbit;
    __asm__ __volatile__ (
        "lock btsl %2,%1\n\tsbbl %0,%0"
        : "=r" (oldbit), "=m" ((*(volatile long *) addr)) : "Ir" (nr) : "memory");
    return oldbit;
}

static __inline__ int synch_test_and_clear_bit(int nr, volatile void * addr)
{
    int oldbit;
    __asm__ __volatile__ (
        "lock btrl %2,%1\n\tsbbl %0,%0"
        : "=r" (oldbit), "=m" ((*(volatile long *) addr)) : "Ir" (nr) : "memory");
    return oldbit;
}

static __inline__ int synch_const_test_bit(int nr, const volatile void * addr)
{
    return ((1UL << (nr & 31)) & 
            (((const volatile unsigned int *) addr)[nr >> 5])) != 0;
}

static __inline__ int synch_var_test_bit(int nr, volatile void * addr)
{
    int oldbit;
    __asm__ __volatile__ (
        "btl %2,%1\n\tsbbl %0,%0"
        : "=r" (oldbit) : "m" ((*(volatile long *) addr)), "Ir" (nr) );
    return oldbit;
}

#define synch_test_bit(nr,addr) \
(__builtin_constant_p(nr) ? \
 synch_const_test_bit((nr),(addr)) : \
 synch_var_test_bit((nr),(addr)))

#endif
