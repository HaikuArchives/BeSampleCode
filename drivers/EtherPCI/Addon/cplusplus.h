/*
 * cplusplus.h
 *
 * Include this file, and then you can use EXTERNC for externs, whether
 * you are compiling C code or C++ code.
 */
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#ifndef _CPLUSPLUS_H
#define _CPLUSPLUS_H

#if __cplusplus
#define EXTERNC extern "C"
#else /* __cplusplus */
#define EXTERNC extern
#endif /* __cplusplus */

#endif /* _CPLUSPLUS_H */


