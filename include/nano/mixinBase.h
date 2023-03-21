//______________________________________________________________________________
// MixinBase functions; called by ethosFoundation's common/assert.h, etc.
// Oct-2011: W. Michael Petullo
//______________________________________________________________________________

#ifndef __MIXIN_BASE_H__
#define __MIXIN_BASE_H__

#include <nano/status.h>
#include <nano/ethosTypes.h>
#include <nano/slice.h>



#ifndef CLANG_ANALYZER_NORETURN
	#if defined(__has_feature)
	#if __has_feature(attribute_analyzer_noreturn)
		#define CLANG_ANALYZER_NORETURN __attribute__((analyzer_noreturn))
	#else
		#define CLANG_ANALYZER_NORETURN
	#endif
	#else
		#define CLANG_ANALYZER_NORETURN
	#endif
#endif

void    mixinExit(int status) CLANG_ANALYZER_NORETURN;
int64   mixinGetTimeOfDay(void);
void    mixinFloatingPointStateSave (void);
bool    mixinXallocUserSpaceInit(void);     // used only in user space
#endif
