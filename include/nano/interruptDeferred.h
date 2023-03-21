//______________________________________________________________________________
// flags to indicate there is deferred interrupt process
//______________________________________________________________________________

#ifndef __INTERRUPT_DEFERRED_H__
#define __INTERRUPT_DEFERRED_H__

// don't need to block interrupt.  Interrupt increments occurred,
// deferred interrupt code increments serviced
// Invariant: occured >= serviced.
typedef struct {
    ulong occured;   // number of interrupts which have occured
    ulong serviced;  // number of interrupts which have been serviced
} InterruptService;

InterruptService timerInterrupt;
InterruptService consoleInterrupt;
InterruptService blkfrontInterrupt;

static inline
ulong
serviceInterrupt(InterruptService is)
{
    return is.occured-is.serviced;
}
#endif
