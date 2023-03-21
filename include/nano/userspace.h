//______________________________________________________________________________
// User space access macros.
//   . checks that pointers from user space point to user space
//   . Copy routines to/from user space
// Mar-2008: Andrei Warkentin
//______________________________________________________________________________

#ifndef __USERSPACE_H__
#define __USERSPACE_H__

#include <ethos/kernel/common.h>
#include <ethos/kernel/arch/memory.h>

// No-check 1/2/4/8-byte access functions, that set *address to store.
int userspace_8to_nocheck(void *address, uint64 store);
int userspace_4to_nocheck(void *address, uint32 store);
int userspace_2to_nocheck(void *address, uint16 store);
int userspace_1to_nocheck(void *address, uchar    store);

// No-check 1/2/4/8-byte access functions, that set *store to *address.
int userspace_8from_nocheck(void *address, uint64 *store);
int userspace_4from_nocheck(void *address, uint32 *store);
int userspace_2from_nocheck(void *address, uint16 *store);
int userspace_1from_nocheck(void *address, uchar *store);

// No-check to/from userspace memcpy.
int userspace_memcpy_nocheck(void *dest, void *source, unsigned int length);

// Checking to-userspace memcpy.
static inline
Status
userspace_memcpy_to(void *udest,
		    void *ksource,
		    msize_t length)
{
    Status status = StatusPageFault;
    if (addressRangeInUserspace((vaddr_t) udest, length))
	{
	    status = userspace_memcpy_nocheck(udest, ksource, length);
	}
    return status;
}

// Checking from-userspace memcpy.
static inline
Status
userspace_memcpy_from(void *kdest, void *usource, msize_t length) 
{ 
    Status status = StatusPageFault;
    if (addressRangeInUserspace((vaddr_t) usource, length))
	{
	    status = userspace_memcpy_nocheck(kdest, usource, length);
	}
    return status;
}

// Non-checking 1/2/4/8-byte access macro.
//  must be a macro since the type of 'where' flows through
#define userspace_from_nocheck(address, where) \
({ \
   int status = StatusPageFault; \
   C_ASSERT(sizeof(*(where)) <= ARCH_INTEGRAL_SIZEOF); \
   switch(sizeof(*(where))) \
   { \
	case sizeof(uint64_t): status = userspace_8from_nocheck((address), (uint64_t*)(where)); break; \
    case sizeof(uint32_t): status = userspace_4from_nocheck((address), (uint32_t*)(where)); break; \
    case sizeof(uint16_t): status = userspace_2from_nocheck((address), (uint16_t*)(where)); break; \
    case sizeof(uchar): status = userspace_1from_nocheck((address), (uchar*)(where)); break; \
    default: break; \
   } \
  status; \
})

// Checking 1/2/4/8-byte access macro
#define userspace_from(address, where) \
({ \
  int status = StatusPageFault; \
  C_ASSERT(sizeof(*(where)) <= ARCH_INTEGRAL_SIZEOF); \
  if(!addressInUserspace((vaddr_t) (address))) \
  { \
    status = StatusPageFault; \
  } \
  else \
  { \
      status = userspace_from_nocheck((address), (where)); \
  } \
  status; \
})

// Non-checking 1/2/4/8-byte access macro.
#define userspace_to_nocheck(address, what) \
({ \
   int status = StatusPageFault; \
   C_ASSERT(sizeof((what)) <= ARCH_INTEGRAL_SIZEOF); \
   switch(sizeof((what))) \
   { \
	case sizeof(uint64_t): status = userspace_8to_nocheck((address), (uint64_t*)(where)); break; \
    case sizeof(uint32_t): status = userspace_4to_nocheck((address), (what)); break; \
    case sizeof(uint16_t): status = userspace_2to_nocheck((address), (what)); break; \
    case sizeof(uchar): status = userspace_1to_nocheck((address), (what)); break; \
    default: break; \
   } \
  status; \
})

// Checking 1/2/4/8-byte access macro
#define userspace_to(address, what) \
({ \
  int status = StatusPageFault; \
  C_ASSERT(sizeof((what)) <= ARCH_INTEGRAL_SIZEOF); \
  if(!is_userspace_address((vaddr_t) (address))) \
  { \
    status = StatusPageFault; \
  } \
  else \
  { \
      status = userspace_to_nocheck((address), (what)); \
  } \
  status; \
})

#endif
