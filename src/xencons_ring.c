//______________________________________________________________________________
/// Xen console routines.
///
// from Mini-OS 
// Modified: Jon A. Solworth  (6/2009)
//______________________________________________________________________________

#include <nano/common.h>
#include <nano/interruptDeferred.h>
#include <nano/kernelLog.h>
#include <nano/mm.h>
#include <nano/ref.h>
#include <nano/time.h>
#include <nano/xenEvent.h>
#include <nano/xenEventHandler.h>
#include <nano/xenSchedule.h>
#include <xen/io/console.h>

static inline struct
xencons_interface *
xencons_interface(void)
{
    return (struct xencons_interface *) mfnToVirtual(start_info.console.domU.mfn);
}

//______________________________________________________________________________
/// notify backend via event to look into the console buffer
//______________________________________________________________________________
static inline void 
xencons_notify_backend(void)
{
    // Use evtchn: this is called early, before irq is set up.
    xenEventSend(start_info.console.domU.evtchn);
}

char *userInput[10];
int count = 0;
char temp[100];
int k = 0;
void
xencons_rx(char *buf, unsigned len)
{
    if (len > 0)
	{
	    // Just repeat what's written 
	    buf[len] = '\0';
		temp[k] = buf[len - 1]; // a
		k++;     
		printf("%s", buf);
		if (buf[len-1] == '\r') {
			printf("\n");
		}   
	}
}


void
xencons_tx(void)
{
    // Do nothing, handled by _rx
}



//______________________________________________________________________________
/// copies over len bytes from data to the console, len should be large enough
/// to include trailing '\0'.  If initialized is true, will send an event.
//______________________________________________________________________________
int
xencons_ring_send(int initialized, const char *data, unsigned len)
{	
    int sent = 0;
    volatile struct xencons_interface *intf = xencons_interface();
    XENCONS_RING_IDX cons, prod;

    // message must fit within the buffer
    REQUIRE(len <= sizeof(intf->out));
    
    // Yes. SEND EVERYTHING.
    while (sent != len)
	{
	    cons = intf->out_cons;
	    prod = intf->out_prod;
	    int bufferSpace = sizeof(intf->out) - (prod - cons);
	    if (bufferSpace>0)
		{     
		    int count = MIN(bufferSpace, (len-sent));
		    int i;
		    for (i=0; i<count; i++)
			intf->out[MASK_XENCONS_IDX(prod++, intf->out)] = data[sent++];
		    wmb();
		    intf->out_prod = prod;                   
		    wmb();
		}      
	    else 
		{ // buffer full, yield
		    xenScheduleYield();
		}
	}

    if (initialized && sent)
	{
	    xencons_notify_backend();
	}

    return sent;
}



//______________________________________________________________________________
/// console interrupt handler
//______________________________________________________________________________
static void
consoleHandler(evtchn_port_t port, arch_interrupt_regs_t *regs, void *ign)
{
    consoleInterrupt.occured++;
    struct xencons_interface *intf = xencons_interface();
    XENCONS_RING_IDX cons, prod;
    cons = intf->in_cons;
    prod = intf->in_prod;
    mb();
    BUG_ON((prod - cons) > sizeof(intf->in));
    while (cons != prod) {
		// char *c = intf->in + MASK_XENCONS_IDX(cons, intf->in);
		xencons_rx(intf->in + MASK_XENCONS_IDX(cons, intf->in), 1);
		cons++;
    }
    mb();
    intf->in_cons = cons;
    xencons_notify_backend();
    xencons_tx();

    consoleDoAll();
}

//______________________________________________________________________________
/// sets up console event channel (apparently console can function without it)
/// and allow for console input (assuming a xm console command has be done on Dom0
//______________________________________________________________________________
int
xencons_ring_init(void)
{
    if (!start_info.console.domU.evtchn)
	{
	    printfLog("Bad evtchn for xen console, xen console failed\n");
	    return 0;
	}

    int err = xenEventBind(start_info.console.domU.evtchn, consoleHandler,  NULL);
    if (err <= 0) 
	{
	    printfLog("XEN console request chn bind failed %i\n", err);
	    return err;
	}

    // In case we have in-flight data after save/restore...
    xencons_notify_backend();

    xencons_flush();

    return 0;
}

//______________________________________________________________________________
/// resume code
//______________________________________________________________________________
void
xencons_resume(void)
{
    xencons_ring_init();
}

//_______________________________________________________________________________
/// flush out console buffer, ensuring backend has received it
//_______________________________________________________________________________
void
xencons_flush(void)
{
    XENCONS_RING_IDX cons, prod;
    volatile struct xencons_interface *intf = xencons_interface();
    while (1)
	{      
	    rmb();
	    cons = intf->out_cons;
	    prod = intf->out_prod;
	    if (cons == prod) 
		{
		    break;
		}

	    xenScheduleYield();
	}  
}
