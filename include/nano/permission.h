//______________________________________________________________________________
// read, write, and execute permissions in Ethos.
//______________________________________________________________________________
#ifndef __ETHOS_PERMISSION_H__
#define __ETHOS_PERMISSION_H__

// Permissions.  These are generic permissions,
// hardware protections need to be mapped form
// these values
typedef unsigned long permission_t;
#define PERM_NONE    (0x0)
#define PERM_READ    (0x1)
#define PERM_WRITE   (0x2)
#define PERM_EXEC    (0x4)
#define PERM_ALL   (PERM_READ | PERM_WRITE | PERM_EXEC)

#endif
