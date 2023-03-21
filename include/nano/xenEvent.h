//______________________________________________________________________________
//
// Xen event handling
//
// (C) 2003 - Rolf Neugebauer - Intel Research Cambridge
// (C) 2005 - Grzegorz Milos - Intel Research Cambridge
// (C) 2008 - Andrei Warkentin
//______________________________________________________________________________

#ifndef __XEN_EVENT_H__
#define __XEN_EVENT_H__

#include <nano/cpuPrivileged.h>
#include <nano/xenEventHandler.h>
#include <xen/event_channel.h>

typedef void (*evtchn_handler_t)(evtchn_port_t, arch_interrupt_regs_t *, void *);

void          xenEventHandle(evtchn_port_t port, arch_interrupt_regs_t *regs);
int           xenEventBindVirq(uint32_t virq, evtchn_handler_t handler, void *data);
evtchn_port_t xenEventBind(evtchn_port_t port, evtchn_handler_t handler,
			  void *data);
void          xenEventUnbind(evtchn_port_t port);
void          xenEventInit(void);
int           xenEventAllocUnbound(domid_t pal, evtchn_handler_t handler,
				   void *data, evtchn_port_t *port);
int           xenEventBindInterdomain(domid_t pal, evtchn_port_t remote_port,
				      evtchn_handler_t handler, void *data,
				      evtchn_port_t *local_port);
void          xenEventUnbindAllPorts(void);
int           xenEventSend(evtchn_port_t);
void
xenEventConsole(evtchn_handler_t handler,    ///< handler for console event
        void *data                   ///< data to be passed to handler
        );

static inline int
notify_remote_via_evtchn(evtchn_port_t port)
{
  evtchn_send_t op;
  op.port = port;
  return HYPERVISOR_event_channel_op(EVTCHNOP_send, &op);
}

// mask events
#define __cli()								\
do {									\
  vcpu_info_t *_vcpu;							\
  _vcpu = &HYPERVISOR_shared_info->vcpu_info[smp_processor_id()];	\
  _vcpu->evtchn_upcall_mask = 1;					\
  barrier();								\
 } while (0)

// unmask events
#define __sti()								\
do {									\
  vcpu_info_t *_vcpu;							\
  barrier();								\
  _vcpu = &HYPERVISOR_shared_info->vcpu_info[smp_processor_id()];	\
  _vcpu->evtchn_upcall_mask = 0;					\
  barrier(); /* unmask then check (avoid races) */			\
  if ( unlikely(_vcpu->evtchn_upcall_pending) )				\
    force_evtchn_callback();						\
 } while (0)

#define __save_flags(x)							\
do {									\
  vcpu_info_t *_vcpu;							\
  _vcpu = &HYPERVISOR_shared_info->vcpu_info[smp_processor_id()];	\
  (x) = _vcpu->evtchn_upcall_mask;					\
 } while (0)

#define __restore_flags(x)						\
do {									\
  vcpu_info_t *_vcpu;							\
  barrier();								\
  _vcpu = &HYPERVISOR_shared_info->vcpu_info[smp_processor_id()];	\
  if ((_vcpu->evtchn_upcall_mask = (x)) == 0) {				\
    barrier(); /* unmask then check (avoid races) */			\
    if ( unlikely(_vcpu->evtchn_upcall_pending) )			\
      force_evtchn_callback();						\
  }									\
 } while (0)

#define __save_and_cli(x)						\
  do {									\
    vcpu_info_t *_vcpu;							\
    _vcpu = &HYPERVISOR_shared_info->vcpu_info[smp_processor_id()];	\
    (x) = _vcpu->evtchn_upcall_mask;					\
    _vcpu->evtchn_upcall_mask = 1;					\
    barrier();								\
  } while (0)

#define local_irq_save(x)	__save_and_cli(x)
#define local_irq_restore(x)	__restore_flags(x)
#define local_save_flags(x)	__save_flags(x)
#define local_irq_disable()	__cli()
#define local_irq_enable()	__sti()

#define irqs_disabled()			\
  HYPERVISOR_shared_info->vcpu_info[smp_processor_id()].evtchn_upcall_mask

#endif
