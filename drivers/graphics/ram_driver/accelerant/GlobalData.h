/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#if !defined(GLOBALDATA_H)
#define GLOBALDATA_H

#include "../include/DriverInterface.h"

extern int fd;
extern shared_info *si;
extern area_id shared_info_area;
extern area_id regs_area;
extern display_mode *my_mode_list;
extern area_id my_mode_list_area;
extern int accelerantIsClone;

#endif
