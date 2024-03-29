//______________________________________________________________________________
// xenEventHandler.c
// 
// Communication to/from hypervisor.
// 
// Copyright (c) 2002-2003, K A Fraser
// Copyright (c) 2005, Grzegorz Milos, gm281@cam.ac.uk,Intel Research Cambridge
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
// DEALINGS IN THE SOFTWARE.
//______________________________________________________________________________


#include <nano/common.h>
#include <nano/xenEventHandler.h>
#include <nano/xenEvent.h>

static inline int
xenEventHandlerActive(int cpu,                    ///< processor
		      struct shared_info *sh,     ///< place where event info is stored
		      int idx                     ///< evebt index
		      )
{
    return sh->evtchn_pending[idx] & ~sh->evtchn_mask[idx];
}


//______________________________________________________________________________
/// do the call back associated with the pseudo interrupt (event channel)
//______________________________________________________________________________
void 
xenEventHandlerCallback(arch_interrupt_regs_t *regs   ///< registers at the time of the event
			)
{
    unsigned long  l1, l2, l1i, l2i;
    unsigned int   port;
    int            cpu = 0;
    shared_info_t *s = HYPERVISOR_shared_info;
    vcpu_info_t   *vcpu_info = &s->vcpu_info[cpu];
   
    vcpu_info->evtchn_upcall_pending = 0;
    
    // NB. No need for a barrier here -- XCHG is a barrier on x86.
    l1 = xchg(&vcpu_info->evtchn_pending_sel, 0);
    while (l1 != 0)
	{
	    l1i = __ffs(l1);
	    l1 &= ~(1 << l1i);        
	    while ((l2 = xenEventHandlerActive(cpu, s, l1i)) != 0)
		{
		    l2i = __ffs(l2);
		    l2 &= ~(1 << l2i);            
		    port = (l1i << 5) + l2i;
		    xenEventHandle(port, regs);
		}
	}
}

//______________________________________________________________________________
/// mask events on the channel port
//______________________________________________________________________________
inline void
xenEventHandlerMask(uint32 port   ///< port to be masked
		    )
{
    shared_info_t *s = HYPERVISOR_shared_info;
    synch_set_bit(port, &s->evtchn_mask[0]);
}

//______________________________________________________________________________
/// allow events on the channel port
//______________________________________________________________________________
inline void 
xenEventHandlerUnmask(uint32 port  ///< port to be unmasked
		      )
{
    shared_info_t *s = HYPERVISOR_shared_info;
    vcpu_info_t *vcpu_info = &s->vcpu_info[smp_processor_id()];

    synch_clear_bit(port, &s->evtchn_mask[0]);

    // The following is basically the equivalent of 'hw_resend_irq'. Just like
    // a real IO-APIC we 'lose the interrupt edge' if the channel is masked.
    if (  synch_test_bit        (port,    &s->evtchn_pending[0]) && 
	  !synch_test_and_set_bit(port>>5, &vcpu_info->evtchn_pending_sel) )
	{
	    vcpu_info->evtchn_upcall_pending = 1;
	    if ( !vcpu_info->evtchn_upcall_mask )
		force_evtchn_callback();
	}
}

//______________________________________________________________________________
/// clear the event channel port
//______________________________________________________________________________
inline void
xenEventHandlerClearPort(uint32 port    ///< port to be cleared
			 )
{
    shared_info_t *s = HYPERVISOR_shared_info;
    synch_clear_bit(port, &s->evtchn_pending[0]);
}
