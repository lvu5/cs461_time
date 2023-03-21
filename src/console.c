/* 
 ****************************************************************************
 * (C) 2006 - Grzegorz Milos - Cambridge University
 ****************************************************************************
 *
 *        File: console.h
 *      Author: Grzegorz Milos
 *     Changes: 
 *              
 *        Date: Mar 2006
 * 
 * Environment: Xen Minimal OS
 * Description: Console interface.
 *
 * Handles console I/O.
 *
 ****************************************************************************
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRNTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */

#include <nano/common.h> 
#include <nano/mm.h>
#include <nano/xenEventHandler.h>
#include <nano/xenEvent.h>
#include <nano/time.h>
#include <nano/ref.h>
#include <nano/kernelLog.h>
#include <xen/io/console.h>

#define CONS_BUF_ORDER   2
#define CONS_BUF_SIZE    ((1<<CONS_BUF_ORDER) * PAGE_SIZE)
#define CONS_BUF_MASK(x) ((x) & (CONS_BUF_SIZE - 1))

// Low level functions defined in xencons_ring.c
extern int xencons_ring_init(void);
extern int xencons_ring_send(int initialzed, const char *data, unsigned len);
extern char temp[100];
extern char *userInput[10];
extern int count;
extern int k;

// If console not initialized the print will be sent to xen serial line 
// NOTE: you need to enable verbose in xen/Rules.mk for it to work. 
bool consoleInitialized = false;

struct ConsoleBufferS {
    char      *data;
    ulong      head,
               tail;
} consoleBuffer = {0,0,0};

static inline 
struct xencons_interface *
xencons_interface(void)
{
	return (struct xencons_interface *)mfnToVirtual(start_info.console.domU.mfn);
}

void
consoleDoAll(void)
{
    volatile struct xencons_interface *intf = xencons_interface();
    XENCONS_RING_IDX cons, prod;
    int bufferSpace = 0;
    unsigned diff;

    if (consoleBuffer.data)
	{
	    // Copy as much of the buffer as will fit into the Xen console buffer
	    diff = consoleBuffer.tail - consoleBuffer.head;
	    diff = CONS_BUF_MASK(consoleBuffer.head) + diff > CONS_BUF_SIZE ?
		CONS_BUF_SIZE - CONS_BUF_MASK(consoleBuffer.head) : diff;

	    if (diff != 0)
		{
		    cons = intf->out_cons;
		    prod = intf->out_prod;
		    bufferSpace = sizeof(intf->out) - (prod - cons);
		    bufferSpace = bufferSpace > diff ? diff : bufferSpace;
		    consoleBuffer.head += xencons_ring_send(consoleInitialized,
							    &consoleBuffer.data[CONS_BUF_MASK(consoleBuffer.head)],
							    bufferSpace);
		}
	}
}

static
int 
console_buffer_send(int initialized, const char *data, unsigned len)
{
    // Copy everything into the buffer, copying into the ring buffer as necessary.
    unsigned end, l = len;
	
    while (l > 0)
	{
	    end = CONS_BUF_SIZE - (consoleBuffer.tail - consoleBuffer.head);
		
	    if (end == 0)
		{
		    consoleDoAll();
		} 
	    else
		{
		    end = l > end ? end : l;
		    end = CONS_BUF_MASK(consoleBuffer.tail) + end > CONS_BUF_SIZE ?
			CONS_BUF_SIZE - CONS_BUF_MASK(consoleBuffer.tail) : end;

		    memcpy(&consoleBuffer.data[CONS_BUF_MASK(consoleBuffer.tail)], data, end);
		    consoleBuffer.tail += end;
		    l -= end;
		    data += end;
		}
	}

	return len;
}

void
consoleFlush(void)
{
    if (consoleBuffer.data) 
	{
	    unsigned diff;
	    while ((diff = consoleBuffer.tail - consoleBuffer.head) != 0) {
		consoleDoAll();
	    }
	}
    xencons_flush();
}

static
void
consolePrintDestination(const char* where, int size)
{
    if (consoleBuffer.data) 
	{
	    console_buffer_send(consoleInitialized, where, size);
	} 
    else 
	{
	    xencons_ring_send(consoleInitialized, where, size);
	}
}

void
consolePrint(const char *data, int length)
{
    int i, last_restart;
    char seq[] = { '\r', '\n' };

    for (i = 0, last_restart = 0; i < length; i++)
	{
	    if (data[i] == '\n') 
		{
		    if (i - last_restart) 
			{
			    consolePrintDestination(data + last_restart, i - last_restart);
			}

		    consolePrintDestination(seq, sizeof(seq));

		    last_restart = i + 1;
		}
	}

    if (last_restart < i) 
	{
	    consolePrintDestination( data + last_restart, i - last_restart);
	}
}


void
consoleBufferInit(void)
{
    // once allocated, never freed
    consoleBuffer.data = (char *) pageKernelAlloc(CONS_BUF_ORDER);
    if (NULL==consoleBuffer.data)
	{
	    xprintLog("Error initializing console buffer...\n");
	    return;
	}

    consoleBuffer.head = consoleBuffer.tail = 0;
}

void handle_input(evtchn_port_t port, arch_interrupt_regs_t *regs, void *data) {
	volatile struct xencons_interface *intf = xencons_interface();
	XENCONS_RING_IDX cons, prod;
	// char *data;
	int len;
	unsigned diff;
	cons = intf->in_cons;
	prod = intf->in_prod;
	xen_rmb();

	while (cons != prod) {
		diff = prod - cons;
		diff = CONS_BUF_MASK(cons) + diff > sizeof(intf->in) ?
			sizeof(intf->in) - CONS_BUF_MASK(cons) : diff;
		// data = (char *)&intf->in[CONS_BUF_MASK(cons)];
		len = diff;
		cons += len;
	}
	 
	intf->in_cons = cons;
	xen_wmb();
}

void
consoleInit(void)
{   
    xencons_ring_init();
    consoleInitialized = true; 
}
