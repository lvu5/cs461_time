//______________________________________________________________________________
//  ref: reference counting objects.
// 
//  Ref object's have a -refCount- and a -pointer- to a vector of objects
//  of a given -type-.
//
//  The -refCount- is the number of pointers to the object.
//  The increment rules are as follows:
//    1. The -refCount- is intialized to one when created
//    2. It should be incremented before being passed to an event stack create.
//    3. It should be incremented before linked into a data structure
//  The decrement rules are similar:
//    1. The -refCount- is decrement in the process which created the object
//       (unless it is passed back as a paramter to a predecessor in the call
//        graph)
//    2. The -refCount- is decremented at the end of the event stack
//    3. The -refCount- is decremented when unlinked to a data structure
//
//  The pointer is to a xalloc object vector for which type and size are maintained.
//
//  Sep-2010: Jon A. Solworth
//______________________________________________________________________________

#ifndef __REF_H__
#define __REF_H__

#include <nano/status.h>
#include <nano/xalloc.h>

Xtype refXtype;

typedef void  (*Free)(void *);

typedef struct {
    ulong      refCount;
    void      *ptr;
} Ref;


#define REFCHECK(_type,_ptr)    ASSERT(refCheck(_type, (Ref*)_ptr))
typedef Ref String;
typedef Ref HashRef;
     
     // allocate a ref object of the specified -type-
     // object can hold at least -minCapacity- elements,
     // but does not currently contain any elements.
     // see -refAppend- for adding elements
Ref *refAllocateEmpty(Xtype type, ulong minCapacity);

     // Creates a ref vector object with specified -type-.
     // The allocated vector contains -minCapacity- storage,
     // initialized using default initialization for the type.
     // see -refCopy- for adding elements
Ref *refAllocate(Xtype type, ulong minCapacity);

     // allocate a ref for a vector of objects, the nubmer of elemnts is -minCapacity-.
     // ptr contains the value to be copied.
Ref *refAllocateInitialize(Xtype type, ulong minCapacity, const void *ptr);

typedef struct MemStruct MemStruct;

Ref *refFromMemStruct(MemStruct *memStruct);

bool refCheck(Xtype type, Ref *ref);

       // Adds another reference to the ref type, boosting
       // the reference count by 1.  This is used to do assignment,
       // decrementing the old ref (if not null) *field, and incrementing
       // -ref-.
Status refHook(Ref **field, Ref *ref);

       // Decrement the reference count, deallocate the field,
       // release the object if refCount goes to 0.  *ref set to NULL.
       // if *ref is NULL, this is a no-op
Status refUnhook(Ref **ref);

       // appends to the ref object if there is sufficient
       // capacity.  Return StatusNoMemory otherwise
Status refAppend(Ref *ref, const void *object, ulong objectCount);

       // copy over -objectCount- elements described in -object- to
       // start of -ref-.
Status refCopy(Ref *ref, const void *object, ulong objectCount);

	// return pointer to the ith -object-
void * refGetElement(Ref *ref, ulong index);

static inline
ulong
refElementCount(Ref *ref)
{
    return xvectorCount(ref->ptr);
}    

static inline
ulong
refSize(Ref *ref)
{
    return xvectorSize(ref->ptr);
}    

Ref   *refIncrement(Ref *ref);


Ref    *stringAllocateEmpty(ulong);
Ref    *stringAllocate(ulong);
Ref    *stringAllocateInitialize(const char*name);

static inline
Ref *
refBufferAllocate(ulong count)       
{
    return refAllocate(ucharXtype, count); 
}

static inline
Ref *
refBufferAllocateEmpty(ulong count)
{ 
    return refAllocateEmpty(ucharXtype, count);
}

static inline
Ref *
refBufferAllocateInitialize(ulong count, const void *initialize)
{
    return refAllocateInitialize(ucharXtype, count, initialize);
}

Status  stringHook(Ref **field, Ref *ref);
Status  stringUnhook(Ref **ref);
void    stringPrint(const char *format, String *s);
bool    stringIsEmpty(String *s);
Status  stringConcat(String *s,
		     void *object,
		     unsigned long size);
int     stringCompare(String *s1, String *s2);
char*   stringTrimTrailing(Ref *s);
String* stringEmpty(void); 

String* stringConcatThree(String *s, char *middle, String *e);

Status  stringExplode(String *string, char sep, String **result[]);
void    stringExplodeDestroy(String *array[]);
String *stringImplode(String *array[], char sep);

static inline char   *stringStr(String *s) { return (char*) s->ptr; }

#endif
