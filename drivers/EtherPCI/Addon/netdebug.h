/*
 * netdebug.h
 *
 * Debugging message support. Nothing really network specific about
 * it, but renamed from "debug.h" to distinguish it from other debug.h's
 * that live around the system, and because the *&^%$#@! Metrowerks 
 * compiler can't deal with subdirs in include directives.
 */
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#ifndef _NETDEBUG_H
#define _NETDEBUG_H

#include <BeBuild.h>
#include "cplusplus.h"

/* 
 * Debug messages can go to a file, to the console, to both or to nowhere.
 * Use debug_setflags() to set.
 */
#define DEBUG_CONSOLE 1
#define DEBUG_FILE 	  2
#define DEBUG_NOFLUSH 4

EXTERNC _IMPEXP_NET void _debug_setflags(unsigned flags);
EXTERNC _IMPEXP_NET unsigned _debug_getflags(void);

#define debug_setflags _debug_setflags
#define debug_getflags _debug_getflags

/* 
 * function     -DDEBUG=?		comment
 * _____________________________________________________
 * wprintf      anything		for warnings
 * dprintf      DEBUG > 0		for debugging messages 
 * ddprintf     DEBUG > 1		for extra debugging messages
 */

EXTERNC _IMPEXP_NET int _wprintf(const char *format, ...);
EXTERNC _IMPEXP_NET int _dprintf(const char *format, ...);

/*
 * Define null function "noprintf"
 */
#if __cplusplus

/* 
 * Thanks, C++ for "inline"!
 */
#if __MWERKS__
/*
 * Stupid metrowerks can't optimize out a null inline function
 */
#define noprintf (void)
#else /* __MWERKS__ */

static inline int noprintf(const char *fmt, ...) { return(1); }

#endif /* __MWERKS__ */

#elif __HIGHC__

/*
 * Thanks, MetaWare for "_Inline"!
 */
static _Inline int noprintf(const char *fmt, ...) { return(1); }

#else /* __cplusplus elif __HIGHC__ */

/*
 * This has the same effect as the above, but it has two problems.
 * 1. It may generate a warning from the compiler about no side effects.
 * 2. It may compile the strings into your program, making it bigger.
 *
 * But, it is ANSI compliant.
 */
#define noprintf (void)

#endif /* __cplusplus elif __HIGHC__ */

#if DEBUG
#define wprintf _dprintf
#define dprintf _dprintf

#if DEBUG > 1
#define ddprintf _dprintf
#else /* DEBUG > 1 */
#define ddprintf noprintf
#endif /* DEBUG > 1 */

#else /* DEBUG */

#define wprintf _wprintf
#define dprintf noprintf
#define ddprintf noprintf

#endif /* DEBUG */

#endif /* _NETDEBUG_H */
