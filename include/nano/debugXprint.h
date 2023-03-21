//______________________________________________________________________________
// DebugXprint functions.
// Apr-2012: W. Michael Petullo
//______________________________________________________________________________

#ifndef __DEBUG_XPRINT_H__
#define __DEBUG_XPRINT_H__

#define debugXprint(category, format, ...)				\
    debugXprintFunc(category, __func__, format, ##__VA_ARGS__)

typedef enum {
    debugTunnel       = 1,
    debugSyscall      = 2,
    debugEvent        = 4,
    debugFd           = 8,
    debugFile         = 16,
    debugLatch        = 32,
    debugProcess      = 64,
    debugEnvelope     = 128,
    debugAuthenticate = 256,
    debugMemory       = 512,
    debugTypes        = 1024,
    debugRpc          = 2048,
    debugIpc          = 4096,
    debugMinimalt     = 8192,
    debugConnect      = 16384,
    debugRekey        = 32768
} DebugFlags;

typedef struct DebugFlagDesc {
    DebugFlags flag;
    char *name;
} DebugFlagDesc;

void  debugXprintOn  (DebugFlags categoryMask);
void  debugXprintOff (DebugFlags categoryMask);
void  debugXprintFunc(DebugFlags category, const char *funcName, char *format, ...);

#endif
