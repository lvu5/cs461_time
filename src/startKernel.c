/******************************************************************************
 * startKernel.c
 *
 * Assorted crap goes here, including the initial C entry point, jumped at
 * from head.S.
 *
 * Copyright (c) 2002-2003, K A Fraser & R Neugebauer
 * Copyright (c) 2005, Grzegorz Milos, Intel Research Cambridge
 * Copyright (c) 2006, Robert Kaiser, FH Wiesbaden
 * Copyright (c) 2008, Andrei Warkentin
 * Copyright (c) 2010, Jon Solworth
 *
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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <nano//common.h>
#include <nano//mm.h>
#include <nano/xenEvent.h>
#include <nano/time.h>
#include <nano/version.h>
#include <nano/xenGrant.h>
#include <nano/initialStore.h>
#include <nano/xenbus.h>
#include <nano/timer.h>
#include <xen/features.h>
#include <xen/version.h>
#include <nano/xenSchedule.h>
#include <nano/blkfront.h>
#include <nano/arg.h>

#include <nano/console.h>
#include <xen/io/console.h>
#include <nano/archPageTable.h>

u8 xen_features[XENFEAT_NR_SUBMAPS * 32];

bool logShadowdaemon = false;
bool consoleImmediate;
char message[100];
char hello[] = "BootstrappingNanoOS...\n";

void masterEventCreateAndExecute(start_info_t *si);

void *malloc(size_t s) { return 0; }
void my_free(void *ptr);
void *my_malloc(size_t size);
void initHeap();
char *my_strcpy(char *dest, const char *src);
void my_printf(char *fmt, ...);
char *my_strncpy(char *dest, const char *src, size_t n);
char *my_strncat(char *dest, const char *src, size_t n);
int my_strcmp(const char *s1, const char *s2);
void *my_memset(void *s, int c, size_t n);
char *my_strcat(char *dest, const char *src);
int my_memcmp(const void *s1, const void *s2, size_t n);
int my_strncmp(const char *s1, const char *s2, size_t n);
extern char temp[100];
extern int k;
// create function pointer point to timeOneShotSet(int64 time)

struct timer_lst {
    void (*timeOneShotSet)(int64 time);
    char *message;
    int index;
    int is_set;
};
//______________________________________________________________________________
/// reports back xen features that ethos may need to know about
//______________________________________________________________________________

void setupXenFeatures(void) {
    xen_feature_info_t fi;
    int i, j;

    for (i = 0; i < XENFEAT_NR_SUBMAPS; i++)
	{
	    fi.submap_idx = i;
	    if (HYPERVISOR_xen_version(XENVER_get_features, &fi) < 0)
		break;

	    for (j=0; j<32; j++)
		xen_features[i*32+j] = !!(fi.submap & 1<<j);
	}
}

void
startKernel(start_info_t *si)
{
    
    // this must be turned on in Xen, standard distributions have it off
    (void) HYPERVISOR_console_io(CONSOLEIO_write, strlen(hello), hello);

    // kernelArgInit((char *)si->cmd_line);

    //debugXprintOn(debugEnvelope);

    archInit(si);
    

    // Set up events.
    xenEventInit();
    
    // Enable event delivery. This is disabled at start of day.
    __sti();

    // Initialize console double-buffering.
    consoleImmediate = true;

    //     initHeap();
    //     char *p1 = (char *)my_malloc(512);
    //     char *p2 = (char *)my_malloc(256);
    //     char *p3 = (char *)my_malloc(1024);
    //     printf("Allocated p1: %p\n", (void *)p1);
    //     my_printf("Allocated p2: %p\n", (void *)p2);
    //     my_printf("Allocated p3: %p\n", (void *)p3);
    //     my_free(p2);
    //     char *p4 = (char *)my_malloc(256);
    //     my_printf("Allocated p4: %p\n", (void *)p4);
    // #ifdef SAFE
    //     my_free(p2);
    // #endif
    //     my_free(p1);
    //     my_free(p3);
    //     my_free(p4);

    // xprintLog(input);
    // xprintLog(hello);
    // xprintLog("input_length: %d", input_length);
    // xprintLog("HELLLOO");
    xprintLog("NanoOS kernel version: $[str]\n", kernel_version);
    xprintLog("NanoOS build info:\n");
    printfLog("start_info:     %p\n",    si);
    printfLog("  nr_pages:     %lu\n",     si->nr_pages);
    printfLog("  shared_inf:   %08lx\n", si->shared_info);
    printfLog("  pt_base:      %p\n",      (void *)si->pt_base);
    printfLog("  mod_start:    0x%lx\n", si->mod_start);
    printfLog("  mod_len:      %lu\n",   si->mod_len);
    printfLog("  flags:        0x%x\n",  (unsigned int)si->flags);
    printfLog("  cmd_line:     %s\n",
	      si->cmd_line ? (const char *)si->cmd_line : "NULL");

    archPrintInfo();
    // get console event channel
    

    // HYPERVISOR_console_io(CONSOLEIO_write, 17, "Enter your name: ");
    // xenEventBind(console_event, handle_input, NULL);
    
    // xencons_resume();
    


    setupXenFeatures();
    consoleInit();
    timeInit();
    pfn_t startPfn = PFN_UP(virtualToPhysical(start_info.pt_base)) + start_info.nr_pt_frames;
    pfn_t maxMappedPfn = (KERN_END - KERN_START) >> PAGE_SHIFT;
    pfn_t maxPfn = start_info.nr_pages;
    archPageTablePopulate(&startPfn, &maxMappedPfn, &maxPfn);
    archPageTableWalk(si->pt_base);
    // create an array of timer_lst
    // struct timer_lst timer_list[10];

    // kernelArgPrint();
    xenScheduleShutdown(0);
    // your code goes here!
    // init();
}
