/* xyz5038.h
 *
 * A module for the XYZ Systems Model 5038 Squarepusher chip.
 *
 * Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */

#ifndef XYZ5038_H
#define XYZ5038_H

#include <module.h>

#define XYZ5038_MODULE_NAME		"generic/xyz5038/v1"

typedef struct {
	module_info		module;

	int32 (*read_foo)();
	int32 (*read_bar)();
	int32 (*write_foo)(int32 new_value);
	int32 (*write_bar)(int32 new_value);
} xyz5038_module_info;

#endif // XYZ5038_H

