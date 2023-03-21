//______________________________________________________________________________
// Grant table manipulation.
// Mar-2008: Andrei Warkentin
//______________________________________________________________________________

#ifndef __XEN_GRANT_H__
#define __XEN_GRANT_H__

#include <xen/grant_table.h>

// NR_GRANT_FRAMES must be less than or equal to that configured in Xen
#define NR_GRANT_FRAMES 8

void          xenGrantInit(void);
grant_ref_t   xenGrantAllocAndGrant(void **map);
grant_ref_t   xenGrantAccess(domid_t domid, ulong frame, int readonly);
grant_ref_t   xenGrantTransfer(domid_t domid, ulong pfn);
ulong         xenGrantEndTransfer(grant_ref_t gref);
int           xenGrantEndAccess(grant_ref_t ref);
const char   *xenGrantOpError(int16_t status);
int           xenGrantUnmapForeignGrant(vaddr_t hostAddr, grant_handle_t handle);
int           xenGrantMapForeignGrant(vaddr_t hostAddr, uint32_t domId, grant_ref_t ref, int readOnly, grant_handle_t* handle);

#endif /* !__XEN_GRANT_H__ */
