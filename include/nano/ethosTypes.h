//______________________________________________________________________________
/// types which are widely through the Ethos kernel
//______________________________________________________________________________

#ifndef __ETHOSTYPES_H__
#define __ETHOSTYPES_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>


typedef uintptr_t         vaddr_t;        // word size of the system, either 32-bit or 64-bit
typedef vaddr_t           msize_t;        // memory sized int
typedef uint64_t          uint64;         // 64-bit unsigned int
typedef uint32_t          uint32;         // 32-bit unsigned int
typedef uint16_t          uint16;         // 16-bit unsigned int
typedef int64_t           int64;          // 64-bit int
typedef int32_t           int32;          // 32-bit int
typedef int16_t           int16;          // 16-bit int
typedef unsigned char     uchar;          // unsigned char

typedef unsigned int      uint;        // unsigned int
typedef unsigned long int ulong;  // unsigned long

typedef int64             Time64;

typedef uint32            ShortHandleId;    // short handle (used for xenbus)
typedef uint64            HandleId;         // Handle 
typedef HandleId          EventId;          // EventId
typedef uint32            EventType;        // to track event processing
typedef HandleId          ProcessId;        // process ID
typedef HandleId          RdId;             // global resource (i.e., file) descriptor
typedef uint32            Fd;               // handle local to a process
typedef uint32            UserCount;
typedef uint32            ExecutableCount;
typedef uint32            Xtype;
typedef uint64            TunnelId;

#define NullHandleId  ((HandleId)0)
#define NullProcessId ((ProcessId)0)
#define NullEventId   ((EventId)0)
#define NullRdId      ((RdId)0)
#define InvalidRdId   (~(RdId)0)


typedef uint32 Status;

int printf(const char *, ...);
void exit(int);


#endif
