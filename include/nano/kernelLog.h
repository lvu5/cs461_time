//______________________________________________________________________________
//  command line arguments to the kernel
//______________________________________________________________________________
#ifndef __KERNEL_LOG_H__
#define __KERNEL_LOG_H__

#include <ethos/kernel/time.h>

enum {
    KernelLogEntryCount      = 1<<14,
    KernelLogEntryBufferSize = 1<<20
};

typedef struct {
    Time64  time;
    uint    start;
    uint    size;
} KernelLogEntry;


uint kernelLogFirst;
uint kernelLogLast;
uint kernelLogBufferEnd;

KernelLogEntry kernelLogEntry[KernelLogEntryCount];
char           kernelLogEntryBuffer[KernelLogEntryBufferSize];

Status fileSystemWriteLog(Time64 time, char *str, uint size);

static inline
Status
kernelLog(const char *str)
{
    uint len = strlen(str) + 1;
    if ((KernelLogEntryBufferSize - kernelLogBufferEnd) < len)
	{ // can't fit at the end of the buffer, start from beginning
	    kernelLogBufferEnd = 0;
	}

    KernelLogEntry *klEntry = kernelLogEntry + kernelLogLast;

    klEntry->time  = timeOfDay64();
    klEntry->start = kernelLogBufferEnd;
    klEntry->size  = len;
    kernelLogLast  = (kernelLogLast + 1) % KernelLogEntryCount; // wrap
    
    memcpy(kernelLogEntryBuffer + kernelLogBufferEnd, str, len);
    kernelLogBufferEnd += len;

    return StatusOk;
}

static inline
Status
kernelLogWrite(void)
{
    while (kernelLogFirst != kernelLogLast)
	{
	    KernelLogEntry *klEntry = kernelLogEntry + kernelLogFirst;
	 
	    fileSystemWriteLog(klEntry->time, kernelLogEntryBuffer + klEntry->start, klEntry->size);

	    kernelLogFirst = (kernelLogFirst + 1) % KernelLogEntryCount;
	}
    return StatusOk;
}

#endif
