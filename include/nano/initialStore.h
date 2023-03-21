//______________________________________________________________________________
// API for manipulating the initial boot storage.
// Mar-2008: Andrei Warkentin
//______________________________________________________________________________
#ifndef __INITSTOR_H__
#define __INITSTOR_H__

#include <nano/ref.h>

// Initializes the initstor API.
Status initialStoreInit(void *start, unsigned long length);

// Returns address and length of requested file.
Status initialStoreFind(const char *name, Ref **returnRef);

#endif
