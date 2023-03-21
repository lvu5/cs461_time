/* -*-  Mode:C; c-basic-offset:4; tab-width:4 -*-
 ****************************************************************************
 * (C) 2003 - Rolf Neugebauer - Intel Research Cambridge
 * (C) 2002-2003 - Keir Fraser - University of Cambridge 
 * (C) 2005 - Grzegorz Milos - Intel Research Cambridge
 * (C) 2006 - Robert Kaiser - FH Wiesbaden
 ****************************************************************************
 *
 *        File: time.c
 *      Author: Rolf Neugebauer and Keir Fraser
 *     Changes: Grzegorz Milos
 *
 * Description: Simple time and timer functions
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

#include <nano/common.h>
#include <nano/cpuPrivileged.h>
#include <nano/memory.h>
#include <nano/xenEvent.h>
#include <nano/time.h>
#include <nano/timer.h>
#include <xen/vcpu.h>
#include <nano/interruptDeferred.h>

//______________________________________________________________________________
// Time functions
//______________________________________________________________________________

// These are periodically updated in shared_info, and then copied here. 
struct shadow_time_info
{
	uint64 tsc_timestamp;    ///< TSC at last update of time vals.
	uint64 system_timestamp; ///< Time, in nanosecs, since boot.
	uint32 tsc_to_nsec_mul;
	uint32 tsc_to_usec_mul;
	int    tsc_shift;
	uint32 version;
};


// copy of wall clock time since The Epoch (00:00:00 UTC, Jan 1, 1970.)
static struct timespec shadow_ts;
static uint32 shadow_ts_version;

Time64 time_passed = 0;
bool message_sent = false;
char *message_in = NULL;
// keeps track of system time (time since last boot) and last tsc
// system time keeps ticking even when VM is not running
static struct shadow_time_info shadow;

#ifndef rmb
#define rmb()  __asm__ __volatile__ ("lock; addl $0,0(%%esp)": : :"memory")
#endif


//______________________________________________________________________________
/// Scale a 64-bit delta by scaling it by shift and multiplying by a 32-bit fraction,
/// yielding a 64-bit result.
//______________________________________________________________________________
static inline
uint64
_scaleDelta(uint64 delta, uint32 mul_frac, int shift)
{
	uint64 product;

	if (shift < 0)
		delta >>= -shift;
	else
		delta <<= shift;

#ifdef __x86_32__
	uint32 tmp1, tmp2;

	__asm__ (
			 "mul  %5       ; "
			 "mov  %4,%%eax ; "
			 "mov  %%edx,%4 ; "
			 "mul  %5       ; "
			 "add  %4,%%eax ; "
			 "xor  %5,%5    ; "
			 "adc  %5,%%edx ; "
			 : "=A" (product), "=r" (tmp1), "=r" (tmp2)
			 : "a" ((uint32)delta), "1" ((uint32)(delta >> 32)), "2" (mul_frac) );
#else
	__asm__ (
			 "mul %%rdx ; shrd $32,%%rdx,%%rax"
			 : "=a" (product) : "0" (delta), "d" ((uint64)mul_frac) );
#endif

	return product;
}

//______________________________________________________________________________
///  get ticks and scale
//______________________________________________________________________________
static
ulong
_nanosecondsSinceLastUpdate(void)
{
	uint64   now = getTsc();        // get the ticks
	uint64 delta = now - shadow.tsc_timestamp;
	return _scaleDelta(delta, shadow.tsc_to_nsec_mul, shadow.tsc_shift);
}

//______________________________________________________________________________
/// get elapse time information since boot from shared info pages.
//______________________________________________________________________________
static
void
_updateTimeFromXen(void)
{
	struct vcpu_time_info *src = &HYPERVISOR_shared_info->vcpu_info[0].time;

	do
		{ // get time, repeat if update occuring while getting time
			shadow.version = src->version;
			rmb();
		    shadow.tsc_timestamp    = src->tsc_timestamp;
			shadow.system_timestamp = src->system_time;
			shadow.tsc_to_nsec_mul  = src->tsc_to_system_mul;
			shadow.tsc_shift        = src->tsc_shift;
			rmb();
		}
	while ((src->version & 1) | (shadow.version ^ src->version));

	shadow.tsc_to_usec_mul = shadow.tsc_to_nsec_mul / 1000;
}

//______________________________________________________________________________
/// Get wall clock time from shared info pages
//______________________________________________________________________________
static
void 
_updateWallclock(void)
{
	shared_info_t *s = HYPERVISOR_shared_info;

	do
		{ // repeat if info being updated when accessed
			shadow_ts_version = s->wc_version;
			rmb();
		    shadow_ts.ts_sec  = s->wc_sec;
			shadow_ts.ts_nsec = s->wc_nsec;
			rmb();
		}
	while ((s->wc_version & 1) | (shadow_ts_version ^ s->wc_version));
}

//______________________________________________________________________________
/// check whether the time version has changed
//______________________________________________________________________________
static inline
void
_timeUpdate(void)
{
	struct vcpu_time_info *src = &HYPERVISOR_shared_info->vcpu_info[0].time;
	shared_info_t *s = HYPERVISOR_shared_info;

	if (shadow.version != src->version) 
		{
			_updateTimeFromXen();
		}

	if  (shadow_ts_version != s->wc_version)
		{
			_updateWallclock();
		}
}


//______________________________________________________________________________
/// timeMonotonic(): returns # of nanoseconds passed since timeInit()
///		Note: This function is required to return accurate
///		time even in the absence of multiple timer ticks.
//______________________________________________________________________________
Time64
timeMonotonic(void)
{
	int64 time;
	uint32 local_time_version;

	do
		{
			local_time_version = shadow.version;
			rmb();
			time = shadow.system_timestamp + _nanosecondsSinceLastUpdate();
			_timeUpdate();
			rmb();
		} while (local_time_version != shadow.version);

	return (Time64)time;
}

//______________________________________________________________________________
/// get time of day
/// NOTE: takes two pointers instead of returning Time struct because this is
/// used by the mixins; the mixin library is built before types, so Time
/// (ETN-defined) is not yet available.
//______________________________________________________________________________
void
timeOfDay(uint32 *seconds, uint32 *nanoseconds)
{
	static time_t oldSeconds = 0;
	static ulong  oldNanoSeconds = 0;

	uint64 nsec = timeMonotonic();
	nsec        += shadow_ts.ts_nsec;
	*seconds     = shadow_ts.ts_sec  + NSEC_TO_SEC(nsec);
	*nanoseconds = nsec % 1000000000UL;

	if ((*seconds < oldSeconds) || ((*seconds == oldSeconds) && (*nanoseconds < oldNanoSeconds)))
		{
			xprintLog("Oops: time went backwards from $[ulong].$[ulong] to $[ulong].$[ulong]\n",
					  oldSeconds, oldNanoSeconds, *seconds, *nanoseconds);
		}
	oldSeconds     = *seconds;
	oldNanoSeconds = *nanoseconds;
}

Time64
timeOfDay64(void)
{
	uint32 seconds;
	uint32 nanoseconds;
	
	timeOfDay(&seconds, &nanoseconds);
	Time64 time = SECONDS(seconds) + nanoseconds;
	return time;
}




//______________________________________________________________________________
// timeOneShotSet: Sets a one-shot timer using a specified delta
//
// delta: The number of nanoseconds in the future when the timer will fire
//______________________________________________________________________________

vcpu_set_singleshot_timer_t *pointer;
void
timeOneShotSet(int64 delta)
{
    // We use the singleshot timer to avoid setting timers for times that
    // have already passed.
    static vcpu_set_singleshot_timer_t op = {0, VCPU_SSHOTTMR_future};
    // Keep trying until we set the timer successfully.
	int result;
    do {
        op.timeout_abs_ns = NOW() + delta;
		pointer = &op;
		printf("future time %lld\n", op.timeout_abs_ns);
		printf("future time %lld\n", pointer->timeout_abs_ns);
		result = HYPERVISOR_vcpu_op(VCPUOP_set_singleshot_timer, 0, &op);
        printfLog("timeOneShotSet: delta = %lld, result = %d\n", delta, result);
    } while (unlikely(result != 0));
}

//______________________________________________________________________________
/// Actions when clock ticks
//______________________________________________________________________________
/// 74 201 520 794 199
/// 74 201 521 248 365
int64 get_input(int64 time) {
	return time;
}

void set_message(char *message) {
	message_sent = true;
	message_in = message;
}

static
void
timeHandler(evtchn_port_t ev,
			arch_interrupt_regs_t *regs,
			void *ign)
{
	// pointer contains the time to set the timer to
	timerInterrupt.occured++;
	// check if timer finished
	if (pointer != NULL){
		if (pointer->timeout_abs_ns < NOW()) {
			if (message_in != NULL)
				printf("%s\n$cs461> ", message_in);
			else
				printf("timer finished\nContinue to type your commands here\n$cs461> ");
		}
		pointer = NULL;
	}
}

//______________________________________________________________________________
/// Bind the timer event so ethos can manage time
//______________________________________________________________________________
void
timeInit(void)
{
	xenEventBindVirq(VIRQ_TIMER, &timeHandler, NULL);
}
