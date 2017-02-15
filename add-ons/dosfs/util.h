/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _DOSFS_UTIL_H_
#define _DOSFS_UTIL_H_

// debugging functions

#ifndef DEBUG
#define ASSERT(c) ((void)0)
#else
int _assert_(char *,int,char *);
#define ASSERT(c) (!(c) ? _assert_(__FILE__,__LINE__,#c) : 0)
#endif

void	dump_bytes(uint8 *buffer, uint32 count);
void	dump_directory(uint8 *buffer);

// time
time_t	dos2time_t(uint32 t);
uint32	time_t2dos(time_t s);

uint8	hash_msdos_name(const char *name);

#define read32(buffer,off) \
	(((uint8 *)buffer)[(off)] + (((uint8 *)buffer)[(off)+1] << 8) + \
	 (((uint8 *)buffer)[(off)+2] << 16) + (((uint8 *)buffer)[(off)+3] << 24))

#define read16(buffer,off) \
	(((uint8 *)buffer)[(off)] + (((uint8 *)buffer)[(off)+1] << 8))

#endif
