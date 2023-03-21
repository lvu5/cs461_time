//______________________________________________________________________________
// xenEventHandler.h
// 
// Hypervisor handling.
// 
//
// Copyright (c) 2002, K A Fraser
// Copyright (c) 2005, Grzegorz Milos
// Updates: Aravindh Puthiyaparambil <aravindh.puthiyaparambil@unisys.com>
// Updates: Dietmar Hahn <dietmar.hahn@fujitsu-siemens.com> for ia64
// Updates: Andrei Warkentin (awarke2@uic.edu) for EthOS.
//______________________________________________________________________________

#ifndef __HYPERVISOR_H__
#define __HYPERVISOR_H__

#include <nano/intTypes.h>
#include <xen/xen.h>
#include <nano/hypercall.h>

#define force_evtchn_callback() ((void)HYPERVISOR_xen_version(0, 0))

//
// a placeholder for the start of day information passed up from the hypervisor
//
union start_info_union
{
    start_info_t start_info;
    char padding[512];
};
extern union start_info_union start_info_union;
#define start_info (start_info_union.start_info)

extern shared_info_t *HYPERVISOR_shared_info;

void xenEventHandlerMask(uint32 port);
void xenEventHandlerUnmask(uint32 port);
void xenEventHandlerClearPort(uint32 port);

#endif 
