//______________________________________________________________________________
// Ethos kernel implementation of mixin functions; called by ethosFoundation's
// common/assert.h, etc.
// Mar-2008: Andrei Warkentin
//______________________________________________________________________________

#include <nano/common.h>
#include <nano/console.h>
#include <nano/time.h>
#include <nano/macros.h>
#include <nano/mixin.h>
#include <nano/debug.h>

void
printLog(const char *str)
{
    return print(str);
}

// Mixin: terminate the kernel.
void
mixinExit(int status)
{
    debugExit(NULL);
}

// Mixin: get the current time.
int64
mixinGetTimeOfDay(void)
{
    return timeOfDay64();
}

