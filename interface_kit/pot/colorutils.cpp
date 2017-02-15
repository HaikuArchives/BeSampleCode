/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <math.h>
#include "colorutils.h"

bool operator==(rgb_color a, rgb_color b)
{
	return ((a.red == b.red) && (a.green == b.green)
		&& (a.blue == b.blue) && (a.alpha == b.alpha));
}

bool operator!=(rgb_color a, rgb_color b)
{
	return (! (a == b));
}

rgb_color make_rgb_color(uint8 red, uint8 green, uint8 blue, uint8 alpha)
{
	rgb_color c;
	c.red = red; c.green = green; c.blue = blue; c.alpha = alpha;
	return c;
} 

rgb_color gradient(rgb_color a, rgb_color b, float value)
{
	if (value <= 0) {
		return a;
	} else if (value >= 1) {
		return b;
	} else {
		rgb_color c;
		int16 dR = b.red - a.red, dG = b.green - a.green, dB = b.blue - a.blue,
			dA = b.alpha - a.alpha;
		c.red = a.red + (int16)floor(value*dR + 0.5);
		c.green = a.green + (int16)floor(value*dG + 0.5);
		c.blue = a.blue + (int16)floor(value*dB + 0.5);
		c.alpha = a.alpha + (int16)floor(value*dA + 0.5);
		return c;
	}
}

