//______________________________________________________________________________
// xtype: dynamically allocated types.  This is the memory allocator
// for all types, subpage and larger within the kernel (or in user space)
//
//  Jon Solworth, Siming Chen, Anna Pablo Dec-2011
//______________________________________________________________________________

#ifndef __XALLOC_H__
#define __XALLOC_H__

#include <nano/ethosTypes.h>
#include <nano/assert.h>

// Macros
#define XVALIDATE(t,p)     ASSERT(xvalidate(t,p))

#define MAX_ORDER          (sizeof(ulong)*8)     // largest slot holding xtype objects and maximum pages for superPage 
#define TYPE_TABLE_ORDER   (10)                  // maximum amount of type 2^-TYPE_TABLE_ORDER-
#define ElementUnallocated ((ElementCount)(1UL<<(8*sizeof(ulong)-1))) 

typedef void (*fptr)(void *ptr);             // function pointer type for printer and initializer functions
typedef ulong ElementCount;                   // for xtype object or array

Xtype   charXtype;
Xtype   ucharXtype;
Xtype   intXtype;
Xtype   ptrXtype;

        // initializes xtypes before first use
bool    xtypeInit(ulong chunkStart, ulong memorySize);

        // create a new type and initialize its type table entry, returns its typeId
Xtype   xtype(char *name, ulong size, uint *minSuperPageOrder, fptr init, fptr ptr);

        // printer wrapper which iterates through all the elements of a type vector
        // pointed by -ptr- and print out the values
void    xtypePrinter(void *ptr);

        // find or allocate a free slot for a specified type;
        // create an initialized object within slot
void*   xalloc(Xtype typeId, ElementCount elementCount);

        // xalloc tuned for character allocation, e.g., malloc
void   *xallocChar(ElementCount elementCount);

        // allocate an empty slot (elementCount is zero)
        // which can hold 2^-order- elements of type -typeId-
void*   xallocEmpty(Xtype typeId, ElementCount elementCount);

        // allocate a vector of -elementCount- and initialize with -initialize-
void*   xallocInitialize(Xtype typeId, ElementCount elementCount, const void *initialize);

        // validates a heap pointer
bool    xvalidate(Xtype typeId, void *p);

        // validates a heap pointer
bool    xvalidatePtr(void *p);

        // free an object or an array within the heap
void    xfree(void *ptrToFree);

        // turn on NoReuse mode for debugging;
        // xtype does not reuse freed memory space
void    xtypeSetNoReuse(void);

        // prints out statistics for a single type
void    xallocStatisticsType(Xtype typeId);

        // prints out memory statistics of all types
void    xallocStatisticsAll(void);

        // prints out the address of a valid heap pointer
        // report error otherwise
void    xtypeAddressPrint(void *ptrToPrint);

        // increase xtype vector pointed by -ptr- by -expandBy- elements
        // and initialize the increased elements
void*   xvectorIncrease(void *ptr, ElementCount expandBy);

void*   xvectorAppend(void *ptr, const void *toAppend, ElementCount elements);

void*   xvectorRealloc(void *ptr, ElementCount size);

ElementCount
        xvectorCapacity(void *p);

        // return the number of elements in xtype vector
        // or ElementUnallocated if not a pointer to 0 or more elements
ElementCount    
        xvectorCount(void *p);

        // size of a vector
ulong   xvectorSize(void *p);

        // size of a single element
ulong   xvectorElementSize(void *p);

bool    xvectorPop(void *ptr, void *element);
void   *xvectorPush(void *ptr, void *element);

ulong   xvectorRemainingElements(void *ptr);

Xtype   xvectorType(void* ptr);

//______________________________________________________________________________
// return -order- of -elementCount-
//______________________________________________________________________________
static inline
uint
logCeil(ElementCount elementCount)
{
    if (!elementCount)
	{
	    return 0;
	}

    // count the number of leading zero bits
    uint nlz   = __builtin_clzl(elementCount);
    uint nbits = sizeof(ElementCount)*8;
    // nunmber of tailing bits after
    uint ntbits = nbits - nlz - 1;

    // only one 1-bit
    if ( ( 1UL << ntbits ) == elementCount )
	{
	    return ntbits;
	}
    else
	{
	    return (ntbits + 1);
	}
}
#endif


