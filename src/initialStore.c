//______________________________________________________________________________
// file: initialStore.c
// Mar-2008: Andrei Warkentin 
//
// this is the initial file which comes up on system boot.
//______________________________________________________________________________

#include <nano/common.h>
#include <nano/initialStore.h>

// A TAR block is 512 bytes.
#define TBLOCK (512)

// TAR file header offsets and field sizes (in bytes).
// File name.
#define TH_NAME       0
#define TH_NAME_SIZE  100

// File mode.
#define TH_MODE       100)
#define TH_MODE_SIZE  8

// UID.
#define TH_UID        108
#define TH_UID_SIZE   8

// GID.
#define TH_GID        116
#define TH_GID_SIZE   8

// File size.
#define TH_SIZE       124
#define TH_SIZE_SIZE  12

// Modification time.
#define TH_TIME       136
#define TH_TIME_SIZE  12

// Checksum.
#define TH_CSUM       148
#define TH_CSUM_SIZE  8

// Link indicator.
#define TH_LINK       156
#define TH_LINK_SIZE  1

// Name of linked file.
#define TH_LNAME      157
#define TH_LNAME_SIZE 100

static const char *initstor_mem_begin = NULL;
static const char *initstor_mem_end = NULL;

//______________________________________________________________________________
// Initializes the initstor API.
//______________________________________________________________________________
Status
initialStoreInit(void *start, unsigned long length)
{
    initstor_mem_begin = (char*) start;
    initstor_mem_end = initstor_mem_begin + length;
    return StatusOk;
}

//______________________________________________________________________________
// Returns address and length of requested file.
//______________________________________________________________________________
Status
initialStoreFind(const char *name, Ref **returnRef)
{
    ASSERT(NULL!=returnRef);
    int status = StatusNotFound;
    const char *seek = initstor_mem_begin;

    *returnRef = NULL;

    if (unlikely(!seek || !initstor_mem_end))
	{
	    return StatusFail;
	}
    while (status == StatusNotFound)
	{
	    ulong file_size = 0;
	    if (initstor_mem_end - seek < TBLOCK)
		{
		    // We're done traversing the archive.
		    goto done;
		}
      
	    // Get the file length. We need it either way.
	    if (sscanf(seek + TH_SIZE, "%" SEXPAND(TH_SIZE_SIZE) "lo", &file_size) == 0)
		{
		    // Couldn't get the file size.
		    status = StatusFail;
		    ASSERT_OK(status);
		    goto done;
		}      

	    if (strncmp(seek + TH_NAME, name, TH_NAME_SIZE) == 0)
		{         
		    String *string = refBufferAllocateInitialize(file_size, seek+TBLOCK);
		    ASSERT(string);
		    status = StatusOk;

		    *returnRef = string;
		    goto done;
		}
	    else
		{
		    seek = seek + TBLOCK + ROUND_UP_ON(file_size, TBLOCK);
		}
	}         
 done:
    return status;
}
