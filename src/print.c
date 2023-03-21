//____________________________________________________________________________
// this implements print.
//____________________________________________________________________________

#include <nano/common.h> 
#include <nano/mm.h>
#include <nano/xenEventHandler.h>
#include <nano/xenEvent.h>
#include <nano/time.h>
#include <nano/ref.h>
#include <nano/kernelLog.h>
#include <xen/io/console.h>

extern bool consoleInitialized;          // used to buffer until initialized
extern bool logShadowdaemon;             // print to log, can lose recent entries
bool        consoleImmediate = false;     // write to the console immediately

void consolePrint(const char *data, int length);

//______________________________________________________________________________
// this is the basic print to console
//______________________________________________________________________________
void
print(const char buf[])
{
    if (consoleImmediate)
	{   // make sure its not lost, but no buffering
	    // may effect timing
	    consolePrint(buf, strlen(buf));
	}
    else
	{
	    // put it into a buffer to write out in master event loop
	    // the use of this kernelLog has two limitations
	    //    1. the kernel may crash before the kernelLog is brought up
	    //    2. the kernel may crash before the buffered kernel log ops
	    //       are written out of the kernel
	    //    3. Debugging the network stack may lead to infinite
	    //       loop as each printed message results in another
	    //       printed message.
	    // if any of these problems occur, define consoleImmediate

	    kernelLog(buf);
	}
}


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
void
printOld(const char buf[])
{
    // static bool _printDirect = false;
    //if (_printDirect && !consoleInitialized)
    //	{
    // (void) HYPERVISOR_console_io(CONSOLEIO_write, strlen(buf), buf);
    //	}
}
