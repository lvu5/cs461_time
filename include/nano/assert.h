//______________________________________________________________________________
// Assert macros; allow a library single library to use ASSERT and run 
// on both Ethos and POSIX. Requires xprintLog.
//
// Mar-2008: Andrei Warkentin
// Oct-2011: W. Michael Petullo
//______________________________________________________________________________

#ifndef __ASSERT_H__
#define __ASSERT_H__

#include <nano/mixinBase.h>

void xprintLog(const char *xfmt, ...);


// Compile-time assert macro.
#define C_ASSERT(e) typedef char __C_ASSERT__[(e)?1:-1] __attribute__((unused)) 

#define BUG()             \
    do                    \
	{                 \
	    xprintLog("BUG($[str]:$[uint])\n", __FILE__, __LINE__); \
	    mixinExit(1); \
	} while (0)

// macro because it uses __FILE__, __LINE__
#define BUG_ON(condition)                                                           \
    do                                                                              \
	{                                                                           \
	    if (condition)                                                          \
		{                                                                   \
		    xprintLog("BUG($[str]:$[uint]): " #condition "\n", __FILE__, __LINE__); \
		    mixinExit(1);                                                   \
		}                                                                   \
	} while(0)

// macro because it uses __FILE__, __LINE__
// Like ASSERT, but not optimized out; use probably indicates error condition
// that should be more carefully handled (i.e., just die for now)
#define REQUIRE(x)					           \
    do                                                             \
	{						           \
	    if (!(x))                                              \
		{ 	                                           \
		    xprintLog("ASSERTION FAILED: $[str] at $[str]:$[uint].\n", \
		              # x ,                                \
		              __FILE__,                            \
		              __LINE__);                           \
		    mixinExit(1);                                  \
		}						   \
	} while(0)


#define REQUIRE_OK(e)						   \
    do								   \
	{							   \
	    if (e)						   \
		{						   \
		    xprintLog("REQUIRE_OK Failed:"			   \
			   " status = $[status] at "		   \
                           "$[str]:$[int]\n",			   \
			   e, __FILE__, __LINE__);		   \
		    mixinExit(1);				   \
		}						   \
	} while (0)


#ifdef WITH_ASSERTS
// macro because it uses __FILE__, __LINE__
#define ASSERT(x)                                                  \
    do                                                             \
	{                                                          \
	    if (!(x))                                              \
		{                                                  \
		    xprintLog("ASSERTION FAILED: $[str] at $[str]:$[uint].\n", \
		              # x ,                                \
		              __FILE__,                            \
		              __LINE__);                           \
		    mixinExit(1);                                  \
		}                                                  \
	} while (0)


#define ASSERT_OK(e)						   \
    do								   \
	{							   \
	    if (e)						   \
		{						   \
		    xprintLog("ASSERT_OK Failed:"			   \
			   " status = $[status] at "		   \
                           "$[str]:$[int]\n",			   \
			   e, __FILE__, __LINE__);		   \
		    mixinExit(1);				   \
		}						   \
	} while (0)
#else
#define ASSERT(x)
#define ASSERT_OK(x)
#endif

#endif
