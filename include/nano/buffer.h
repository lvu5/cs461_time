//______________________________________________________________________________
// Buffer code for aligned character/page buffers
//
//______________________________________________________________________________

#ifndef __BUFFER_H__
#define __BUFFER_H__

void   bufferInit(void);
void*  bufferAllocate(ulong size);
void*  bufferAllocateEmpty(ulong size);
void   bufferFree(void *ptr);

#endif
