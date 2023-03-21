//______________________________________________________________________________
// Handle table code.
// Unique IDs for processes, portals, events. 
//
// Uses 64-bit Ids so an ID is never reused.
//
// Mar-2008: Andrei Warkentin
//______________________________________________________________________________

#ifndef __HANDLE_H__
#define __HANDLE_H__

#include <nano/ethosTypes.h>

// Handle table entry definition.
struct HandleTableEntryS
{
    HandleId handleId;
    void    *reference;
};

// Handle table type.
typedef struct HandleTableEntryS HandleTableEntry;

typedef struct HandleTableS
{
    HandleId              lastAllocated;
    msize_t               used;
    msize_t               size;
    HandleTableEntry     *entry;
} HandleTable;

//______________________________________________________________________________
// Handle table APIs map IDs to references (void *).  The IDs are
// allocated by the handle code.  HandleIds are 64-bits long and
// hence will not repeat under conservative estimates of uptime.
//______________________________________________________________________________

// Initializes a statically-allocated
// handle table structure.
Status handleTableInit(HandleTable *table);

// Allocates a handle given a context to associate
// with the handle.
Status handleAllocate(HandleTable *table, void *reference, HandleId *handleId);

// Frees a handle.
Status handleFree(HandleTable *table, HandleId handleId);

// Returns the context associated with an allocated handle.
Status handleGetReference(const HandleTable *table, HandleId handle, void **reference);

// Converts a short handle ID into a (long) HnadleId.
Status shortHandleToHandleId(const HandleTable *table, ShortHandleId shortHoandId, HandleId *returnHandleId);
#endif
