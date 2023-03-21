/* 
 ****************************************************************************
 * (C) 2006 - Grzegorz Milos - Cambridge University
 * (C) 2008 - Andrei Warkentin - University of Illinois at Chicago
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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */
#ifndef __LIB_CONSOLE_H__
#define __LIB_CONSOLE_H__

#include <nano/cpuPrivileged.h>
#include <stdarg.h>

void xencons_rx(char *buf, unsigned len);
void xencons_tx(void);
void xencons_flush(void);
void consoleDoAll(void);
void consoleFlush(void);
void consoleBufferInit(void);
void consoleInit(void);
void consoleHandlerDeferred(void);
void handle_input(evtchn_port_t port, arch_interrupt_regs_t *regs, void *data);
void consolePrint(const char *data, int length);
extern char temp[100];
extern char *userInput[10];
extern int count;
extern int k;

#endif /* __LIB_CONSOLE_H__ */
