/* -*-  Mode:C; c-basic-offset:4; tab-width:4 -*-
 ****************************************************************************
 * (C) 2003 - Rolf Neugebauer - Intel Research Cambridge
 ****************************************************************************
 *
 *        File: types.h
 *      Author: Rolf Neugebauer (neugebar@dcs.gla.ac.uk)
 *     Changes: 
 *              
 *        Date: May 2003
 * 
 * Environment: Xen Minimal OS
 * Description: a random collection of type definitions
 *
 * Updates: Andrei Warkentin (awarke2@uic.edu) for EthOS
 ****************************************************************************
 * $Id: h-insert.h,v 1.4 2002/11/08 16:03:55 rn Exp $
 ****************************************************************************
 */

// FIXME: We should get rid of this and use C99 types (e.g., uint32_t).

#ifndef __INT_TYPES_H__
#define __INT_TYPES_H__

typedef signed char         s8;
typedef unsigned char       u8;
typedef signed short        s16;
typedef unsigned short      u16;
typedef signed int          s32;
typedef unsigned int        u32;
#ifdef __x86_32__
typedef signed long long    s64;
typedef unsigned long long  u64;
#elif defined(__x86_64__) 
typedef signed long         s64;
typedef unsigned long       u64;
#endif

/* FreeBSD compat types */
typedef unsigned char       u_char;
typedef unsigned int        u_int;
typedef unsigned long       u_long;
#ifdef __x86_32__
typedef long long           quad_t;
typedef unsigned long long  u_quad_t;
typedef unsigned int        uintptr_t;


#elif defined(__x86_64__) 
typedef long                quad_t;
typedef unsigned long       u_quad_t;
typedef unsigned long       uintptr_t;

#endif /* __x86_32__ || __x86_64__ */

typedef  u8 uint8_t;
typedef  s8 int8_t;
typedef u16 uint16_t;
typedef s16 int16_t;
typedef u32 uint32_t;
typedef s32 int32_t;
typedef u64 uint64_t;
typedef s64 int64_t;

#define INT_MAX         ((int)(~0U>>1))
#define UINT_MAX            (~0U)


// Make sure gcc doesn't try to be clever and move things around
// on us. We need to use _exactly_ the address the user gave us,
// not some alias that contains the same information.
typedef struct { volatile int counter; } atomic_t;

#endif 
