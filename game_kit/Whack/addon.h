/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

/* addon.h */

#ifndef ADDON_H
#define ADDON_H

#include <DirectWindow.h>
#include <List.h>

typedef void (*frame_whacker)(
			int32 frame_num,
			int32 *pixel_count,
			uchar *framebuffer,
			int32 bytes_per_row,
			clipping_rect *clipping_rects,
			int32 nrects,
			clipping_rect window_bounds,
			color_space mode);

#define DEFAULT_NAME		"_whacker_"

struct whack_addon
{
	char			path[B_PATH_NAME_LENGTH];
	char 			name[B_OS_NAME_LENGTH];
	char			expression[1024];
	image_id		image;
	frame_whacker	whacker;
};

whack_addon *build_addon(const char *expression);
whack_addon *load_addon(const char *path);
void destroy_addon(whack_addon *addon);
status_t save_addon(whack_addon *addon, const char *directory);

#define CLEANING_FREQUENCY			10
void clean_temporary_files(void);

#endif /* ADDON_H */
