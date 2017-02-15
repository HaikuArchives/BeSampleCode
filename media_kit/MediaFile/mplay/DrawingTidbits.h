#ifndef __DRAWING_TIBITS__
#define __DRAWING_TIBITS__

#include <GraphicsDefs.h>

rgb_color ShiftColor(rgb_color , float );

bool operator==(const rgb_color &, const rgb_color &);
bool operator!=(const rgb_color &, const rgb_color &);

inline rgb_color
Color(int32 r, int32 g, int32 b, int32 alpha = 255)
{
	rgb_color result;
	result.red = r;
	result.green = g;
	result.blue = b;
	result.alpha = alpha;

	return result;
}

const rgb_color kWhite = { 255, 255, 255, 255};
const rgb_color kBlack = { 0, 0, 0, 255};

const float kDarkness = 1.06;
const float kDimLevel = 0.6;

void ReplaceColor(BBitmap *bitmap, rgb_color from, rgb_color to);
void ReplaceTransparentColor(BBitmap *bitmap, rgb_color with);

#endif