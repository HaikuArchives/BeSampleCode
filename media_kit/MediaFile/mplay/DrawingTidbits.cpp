#include <Bitmap.h>
#include <Debug.h>
#include <Screen.h>

#include "DrawingTidbits.h"

inline uchar
ShiftComponent(uchar component, float percent)
{
	// change the color by <percent>, make sure we aren't rounding
	// off significant bits
	if (percent >= 1)
		return (uchar)(component * (2 - percent));
	else
		return (uchar)(255 - percent * (255 - component));
}

rgb_color
ShiftColor(rgb_color color, float percent)
{
	rgb_color result = {
		ShiftComponent(color.red, percent),
		ShiftComponent(color.green, percent),
		ShiftComponent(color.blue, percent),
		0
	};
	
	return result;
}

static bool
CompareColors(const rgb_color a, const rgb_color b)
{
	return a.red == b.red
		&& a.green == b.green
		&& a.blue == b.blue
		&& a.alpha == b.alpha;
}

bool 
operator==(const rgb_color &a, const rgb_color &b)
{
	return CompareColors(a, b);
}

bool 
operator!=(const rgb_color &a, const rgb_color &b)
{
	return !CompareColors(a, b);
}

void
ReplaceColor(BBitmap *bitmap, rgb_color from, rgb_color to)
{
	ASSERT(bitmap->ColorSpace() == B_COLOR_8_BIT); // other color spaces not implemented yet
	
	BScreen screen(B_MAIN_SCREEN_ID);
	uint32 fromIndex = screen.IndexForColor(from);
	uint32 toIndex = screen.IndexForColor(to); 
	
	uchar *bits = (uchar *)bitmap->Bits();
	int32 bitsLength = bitmap->BitsLength();	
	for (int32 index = 0; index < bitsLength; index++) 
		if (bits[index] == fromIndex)
			bits[index] = toIndex;
}

void 
ReplaceTransparentColor(BBitmap *bitmap, rgb_color with)
{
	ASSERT(bitmap->ColorSpace() == B_COLOR_8_BIT); // other color spaces not implemented yet
	
	BScreen screen(B_MAIN_SCREEN_ID);
	uint32 withIndex = screen.IndexForColor(with); 
	
	uchar *bits = (uchar *)bitmap->Bits();
	int32 bitsLength = bitmap->BitsLength();	
	for (int32 index = 0; index < bitsLength; index++) 
		if (bits[index] == B_TRANSPARENT_8_BIT)
			bits[index] = withIndex;
}

