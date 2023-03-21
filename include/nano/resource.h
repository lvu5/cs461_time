//______________________________________________________________________________
// May-2008: Andrei Warkentin
// Generic resource definition. "resources" are entities like file descriptors, etc,
// and provide an abstract interface by which these entities are cloned, freed, etc.
//______________________________________________________________________________

#ifndef __RESOURCE_H__
#define __RESOURCE_H__

#include <ethos/kernel/process.h>

struct resource_s;

// Clone callback - what to do on a process clone.
typedef int (*resource_clone_t) (struct resource_s *this, Process *clone);

// Exec callback - what to do when the process changes process image.
// On a non StatusOkema return status, the close callback is invoked and 
// the resource cleaned up.
typedef int (*resource_exec_t) (struct resource_s *this);

// Close callback - what to do on resource closing (which happens when a process is terminated, for example).
typedef void (*resource_close_t) (struct resource_s *this);

// Resource definition.
typedef struct resource_s
{
  
  // Context.
  void *context;

  // Callback.
  resource_clone_t clone;
  resource_close_t close;
  resource_exec_t exec;

  // For tying together into the process resource list.
  ListHead resourceList;  
} resource_t;

// Initializes resource management.
void resource_t_init(void);

// Creates a new resource.
Status resource_t_create(resource_clone_t clone, resource_close_t close, resource_exec_t exec, void *context, Process *process);

// Clones a resource.
Status resource_t_clone(resource_t *resource, Process *clone);

// Deletes a resource.
void  resource_t_delete(resource_t *resource);

// Exec-callback.
void resource_t_exec(resource_t *resource);

// Print a resource
void resourcePrint(void *ptr);

#endif
