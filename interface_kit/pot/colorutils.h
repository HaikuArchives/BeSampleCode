/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _colorutils_h
#define _colorutils_h

#include <GraphicsDefs.h>

#if B_LITTLE_ENDIAN
#define B_RGBA32_HOST B_RGBA32_LITTLE
#else
#define B_RGBA32_HOST B_RGBA32_BIG
#endif

bool operator==(rgb_color a, rgb_color b);
bool operator!=(rgb_color a, rgb_color b);

rgb_color make_rgb_color(uint8 red, uint8 green, uint8 blue,
	uint8 alpha = 0);
rgb_color gradient(const rgb_color a, const rgb_color b, float val);

typedef enum {
	CHANNEL_RED,
	CHANNEL_GREEN,
	CHANNEL_BLUE,
	CHANNEL_ALPHA
} color_channel;

const rgb_color PURE_WHITE		= { 255, 255, 255, 255 };
const rgb_color PURE_BLACK		= { 0, 0, 0, 255 };

#endif /* _colorutils_h */