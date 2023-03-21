/* -*-  Mode:C; c-basic-offset:4; tab-width:4 -*-
 ****************************************************************************
 * (C) 2003 - Rolf Neugebauer - Intel Research Cambridge
 * (C) 2005 - Grzegorz Milos - Intel Research Cambridge
 ****************************************************************************
 *
 *        File: time.h
 *      Author: Rolf Neugebauer (neugebar@dcs.gla.ac.uk)
 *     Changes: Grzegorz Milos (gm281@cam.ac.uk)
 *              Robert Kaiser (kaiser@informatik.fh-wiesbaden.de)
 *              
 *        Date: Jul 2003, changes: Jun 2005, Sep 2006
 * 
 * Environment: Xen Minimal OS
 * Description: Time and timer functions
 *
 ****************************************************************************
 */

#ifndef __KERNEL_TIME_H__
#define __KERNEL_TIME_H__

#define NOW()                   ((Time64) timeMonotonic())

// wall clock time 
typedef long time_t;

struct timespec {
    time_t      ts_sec;
    long        ts_nsec;
};

// prototypes 
void     timeInit(void);
Time64   timeMonotonic(void);
void     timeOfDay(uint32 *seconds, uint32 *nanoseconds);
Time64   timeOfDay64(void);
void     timeOneShotSet(int64 delta);
int64    get_input(int64 time);
void     set_message(char* message);
//______________________________________________________________________________
// System Time
// 64-bit signed value containing the nanoseconds elapsed since boot time.
// This value is adjusted by frequency drift.
// NOW() returns the current time.
// The other macros are for convenience to approximate short intervals
// of real time into system time
//______________________________________________________________________________
#define ONE_MICROSECOND         (      1000LL)
#define ONE_MILLISECOND         (   1000000LL)
#define ONE_SECOND              (1000000000LL)
#define ONE_YEAR_IN_SECONDS     ((Time64) (3600*24*365))

#define SECONDS(_s)             (((Time64)(_s))  * ONE_SECOND )
#define TENTHS(_ts)             (((Time64)(_ts)) * 100000000LL )
#define HUNDREDTHS(_hs)         (((Time64)(_hs)) * 10000000LL )
#define MILLISECS(_ms)          (((Time64)(_ms)) * ONE_MILLISECOND )
#define MICROSECS(_us)          (((Time64)(_us)) * ONE_MICROSECOND )
#define TimeMax                 ((Time64) 0x7fffffffffffffffLL)
#define FOREVER                 TimeMax
#define NSEC_TO_USEC(_nsec)     (_nsec / 1000LL)
#define NSEC_TO_SEC(_nsec)      (_nsec / ONE_SECOND)

    inline static Time64 timeFromSeconds(int64 s) { Time64 t = SECONDS(s); return t; }

#endif /* __KERNEL_TIME_H__ */
