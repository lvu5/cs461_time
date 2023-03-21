//_________________________________________________________________________
/// Xen event handling
///   events are an abstraction on interrupts and can
///   be based on physical or virtual interrupts,
///   inter-domain or inter-processor communication.
//
// 
// (C) 2003 - Rolf Neugebauer - Intel Research Cambridge
// (C) 2005 - Grzegorz Milos - Intel Reseach Cambridge
// (C) 2008 - Andrei Warkentin 
//_________________________________________________________________________

#include <nano/common.h>
#include <nano/mm.h>
#include <nano/xenEventHandler.h>
#include <nano/xenEvent.h>
#include <nano/fmt.h>

#define NR_EVS 1024

// this represents an event handler. Chaining or sharing is not allowed.
typedef struct _ev_action_t 
{
    evtchn_handler_t  handler;
    void             *data;
    u32               count;
} ev_action_t;

static ev_action_t ev_actions[NR_EVS];
void xenEventDefaultHandler(evtchn_port_t port, arch_interrupt_regs_t *regs, void *data);

static ulong bound_ports[NR_EVS/(8*sizeof(unsigned long))];

//______________________________________________________________________________
/// close each port
//______________________________________________________________________________
void
xenEventUnbindAllPorts(void)
{
    int i;
    
    for (i = 0; i < NR_EVS; i++) // foreach event
	{
	    if (test_and_clear_bit(i, bound_ports))
		{
		    struct evtchn_close close;
		    xenEventHandlerMask(i);
		    close.port = i;
		    HYPERVISOR_event_channel_op(EVTCHNOP_close, &close);
		}
	}
}
  
//_____________________________________________________________________
/// Demux events to different handlers.
//_____________________________________________________________________
void
xenEventHandle(evtchn_port_t port,           ///< event to be handled
	       arch_interrupt_regs_t *regs   ///< registers at time of event
	       )
{
    ev_action_t *action;
    ASSERT(regs);
    ASSERT(port < NR_EVS);
    if (port >= NR_EVS)
	{
	    printfLog("port (0x%x) >= NR_EVS (0x%x)\n", port, NR_EVS);
	    goto out;
	}
    action = &ev_actions[port];
    action->count++;
    
    // Call the handler.
    action->handler(port, regs, action->data);    
 out:
    xenEventHandlerClearPort(port);
}

//______________________________________________________________________________
/// provide an event handler for a port (and also some data)
//______________________________________________________________________________
evtchn_port_t
xenEventBind(evtchn_port_t port,               ///< event to bind to a handler
	     evtchn_handler_t handler,         ///< the procedure to call on the event
	     void *data                        ///< data to be associated with handler
	     )
{
    if (ev_actions[port].handler != xenEventDefaultHandler)
	{
	    printfLog("WARN: Handler for port %d already registered, replacing\n",
		   port);
	}

    ev_actions[port].data = data;
    wmb();
    ev_actions[port].handler = handler;

    // Finally unmask the port 
    xenEventHandlerUnmask(port);

    return port;
}

//______________________________________________________________________________
/// free an event channel port
//______________________________________________________________________________
void
xenEventUnbind(evtchn_port_t port            ///< remove the handler on a port
		 )
{
    if (ev_actions[port].handler == xenEventDefaultHandler)
	{
	    printfLog("WARN: No handler for port %d when unbinding\n", port);
	}
    ev_actions[port].handler = xenEventDefaultHandler;
    wmb();
    ev_actions[port].data = NULL;
}

//______________________________________________________________________________
/// bind a virtual irq to a handler
//______________________________________________________________________________
int
xenEventBindVirq(uint32_t virq,               ///< virtual irq
		 evtchn_handler_t handler,    ///< handler for the virq
		 void *data                   ///< data to be passed to handler
		 )
{
    evtchn_bind_virq_t op;

    // Try to bind the virq to a port
    op.virq = virq;
    op.vcpu = smp_processor_id();

    if ( HYPERVISOR_event_channel_op(EVTCHNOP_bind_virq, &op) != 0 )
	{
	    printfLog("Failed to bind virtual IRQ %d\n", virq);
	    return 1;
	}

    set_bit(op.port,bound_ports);
    xenEventBind(op.port, handler, data);
    return 0;
}

#if defined(__x86_64__)

// Allocate 4 pages for the irqstack
#define STACK_PAGES 4
char irqstack[1024 * 4 * STACK_PAGES];

static struct pda
{
    int irqcount;       // offset 0 (used in x86_64.S)
    char *irqstackptr;  //        8 
} cpu0_pda;

#endif


//________________________________________________________________________
/// Initially all events are without a handler and disabled
//________________________________________________________________________
void
xenEventInit(void)
{
    int i;
#if defined(__x86_64__)
    asm volatile("movl %0,%%fs ; movl %0,%%gs" :: "r" (0));
    wrmsrl(0xc0000101, &cpu0_pda); /* 0xc0000101 is MSR_GS_BASE */
    cpu0_pda.irqcount = -1;
    cpu0_pda.irqstackptr = irqstack + 1024 * 4 * STACK_PAGES;
#endif

    // inintialise event handler
    for ( i = 0; i < NR_EVS; i++ )
	{
	    ev_actions[i].handler = xenEventDefaultHandler;
	    xenEventHandlerMask(i);
	}
}

//______________________________________________________________________________
/// this is the default handler, just prints a message and returns
//______________________________________________________________________________
void
xenEventDefaultHandler(evtchn_port_t port,             ///< port causing handler to be called
			  arch_interrupt_regs_t *regs,    ///< registers at time of event
			  void *ignore                    ///< ignored data
			  )
{
    printfLog("[Port %d] - event received\n", port);
}

//________________________________________________________________________
/// Create a port available to the pal for exchanging notifications.
/// Returns the result of the hypervisor call.
//
//  Typically, pal would be notified through the XenStore, so that they
//  could do an xenEventBindInterdomain().

/// Unfortunate confusion of terminology: the port is unbound as far
/// as Xen is concerned, but we automatically bind a handler to it
///   from inside ethos.
//________________________________________________________________________
int
xenEventAllocUnbound(domid_t pal,                 ///< other domain for event channel
			evtchn_handler_t handler,    ///< handler for this domain
			void *data,                  ///< data to be given when event handler calls
			evtchn_port_t *port          ///< port allocated for this domain
			)
{
    evtchn_alloc_unbound_t op;
    op.dom = DOMID_SELF;
    op.remote_dom = pal;
    int err = HYPERVISOR_event_channel_op(EVTCHNOP_alloc_unbound, &op);
    if (err)
	return err;
    *port = xenEventBind(op.port, handler, data);
    return err;
}

//________________________________________________________________________
/// Connect to a port created by pal, allowing the exchange of notifications
/// with the pal. Returns the result of the hypervisor call.
//________________________________________________________________________
int
xenEventBindInterdomain(domid_t pal,                 ///< other domain communicated with
			evtchn_port_t remote_port,   ///< other doamin port
			evtchn_handler_t handler,    ///< handler on interdomain event
			void *data,                  ///< extra informatin
			evtchn_port_t *local_port    ///< ethos port
			)
{
    evtchn_bind_interdomain_t op;
    op.remote_dom = pal;
    op.remote_port = remote_port;
    int err = HYPERVISOR_event_channel_op(EVTCHNOP_bind_interdomain, &op);
    if (err)
	return err;
    set_bit(op.local_port, bound_ports);
    evtchn_port_t port = op.local_port;
    xenEventHandlerClearPort(port);	      // Without, handler gets invoked now!
    *local_port = xenEventBind(port, handler, data);
    return err;
}

//______________________________________________________________________________
/// send an event
//______________________________________________________________________________
int
xenEventSend(evtchn_port_t port ///< Port on which to send event
	       )
{
    struct evtchn_send event;
    event.port = port;
    int err = HYPERVISOR_event_channel_op(EVTCHNOP_send, &event);
    return err;
}

