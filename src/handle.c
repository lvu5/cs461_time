//______________________________________________________________________________
/// Handle table code.
/// We need unique IDs for proceses, portals, events, etc...
/// The handle code keeps track of those IDs which are still and use, and the object
/// to which they map.
//
//  Handles are guaranteed to never repeat.  (Well, this is not mathematically true,
//  but it would take so long to repeat as to be uninteresting when it does.)
//
// Mar-2008: Andrei Warkentin
// Jul-2009: Jon A. Solworth
//______________________________________________________________________________

#include <nano/core.h>
#include <nano/ctype.h>
#include <nano/status.h>
#include <nano/assert.h>
#include <nano/list.h>
#include <nano/handle.h>
#include <nano/xtype.h>
#include <nano/macros.h>
#include <nano/xalloc.h>

const HandleId HandleTableMinimumSize = 1024;  // must be a power of 2

// A free handle has no reference
static inline
bool
_handleIsFree(const HandleTable *table, HandleId handleId)
{
    return !(table->entry[handleId % table->size].reference);
}

//______________________________________________________________________________
/// No need to dynamically allocate, then number of HandleTables is fixed in the
/// kernel.
//______________________________________________________________________________
Status
handleTableInit(HandleTable *table
		)
{
    memzero(table, sizeof(*table));

    // pre-allocate a bunch of memory
    table->size        = HandleTableMinimumSize;
    ASSERT(handleTableEntryXtype);
    table->entry       = (HandleTableEntry*) xalloc(handleTableEntryXtype, HandleTableMinimumSize);
    if (!table->entry)
	{
	    return StatusNoMemory;
	}
    memzero(table->entry, HandleTableMinimumSize * sizeof(HandleTableEntry));
    
    return StatusOk;
}

//______________________________________________________________________________
/// Returns the reference associated with a handle.
//______________________________________________________________________________
Status
handleGetReference(const HandleTable *table,
		   HandleId handleId,
		   void **reference)
{
    ASSERT(table);
    ASSERT(table->entry);

    if (!handleId)
	{
	    return StatusInvalidHandle;
	}
    if (handleId > table->lastAllocated)
	{
	    return StatusInvalidHandle;
	}

    if (table->entry[handleId % table->size].handleId == handleId)
	{   // foundit 
	    *reference = table->entry[handleId % table->size].reference;
	    return StatusOk;
	}
    else
	{
	    *reference = NULL;
	    return StatusNotFound;
	}
}

//______________________________________________________________________________
/// Frees a handle.
//______________________________________________________________________________
Status
handleFree(HandleTable *table,
	   HandleId handleId)
{
    ASSERT(table);
    ASSERT(table->entry);

    if (table->entry[handleId % table->size].handleId == handleId)
	{ // Found it, now nullify it
	    table->entry[handleId % table->size].handleId  = (HandleId) 0;
	    table->entry[handleId % table->size].reference = NULL;
	    table->used--;

	    return StatusOk;
	}

    return StatusInvalidHandle;
}

//______________________________________________________________________________
/// Double the size of the handle table
//______________________________________________________________________________
Status
handleTableGrow(HandleTable *table)
{
    msize_t newSize = 2*table->size; // table size always a power of 2
    HandleTableEntry *newStart;
    newStart = (HandleTableEntry *) xalloc(handleTableEntryXtype, newSize);
    if (unlikely(!newStart))
	{
	    return StatusNoMemory;
	}

    memzero(newStart, sizeof(HandleTableEntry) * newSize);

    int i;
    HandleId handleId;
    for (i=0; i<table->size; i++)
	{   // copy handles, note that handle may end in a different slot
	    handleId = table->entry[i].handleId;
	    newStart[handleId % newSize].reference = table->entry[i].reference;
	    newStart[handleId % newSize].handleId  = handleId;
	}
    xfree(table->entry);
    table->entry = newStart;
    table->size  = newSize;

    return StatusOk;
}

//______________________________________________________________________________
/// Allocates a handle.
//______________________________________________________________________________
Status
handleAllocate(HandleTable *table,
	       void *reference,
	       HandleId *returnHandleId)
{
    ASSERT(table);
    ASSERT(reference);
    ASSERT(returnHandleId);

    HandleId handleId;
    Status status;

    *returnHandleId = NullHandleId;

    // the table size is kept between 1/4 and 1/2 full, resulting in the time
    // to get a new handle being small.
    if (table->size < 2*table->used)
	{
	    status = handleTableGrow(table);
	    if (StatusOk!=status)
		{
		    return status;
		}
	}

    // above conditional ensures that the table always has many free handles
    // so that the below loop terminates and does so in expected constant time
    while (1)
	{
	    table->lastAllocated++;
	    if (_handleIsFree(table, table->lastAllocated))
		{
		    *returnHandleId = handleId = table->lastAllocated;
		    table->entry[handleId % table->size].reference = reference;
		    table->entry[handleId % table->size].handleId  = handleId;
		    table->used++;
		    break;
		}
	}


    return StatusOk;
}

Status
shortHandleToHandleId(const HandleTable *table, ShortHandleId shortHandleId, HandleId *returnHandleId)
{
    HandleId handleId = shortHandleId;

    if (_handleIsFree(table, handleId))
	{
	    return StatusNotFound;
	}
    handleId = table->entry[shortHandleId % table->size].handleId;

    *returnHandleId = handleId;

    return StatusOk;
}
