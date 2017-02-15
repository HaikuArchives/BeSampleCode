/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef __COLOR_LABEL__
#define __COLOR_LABEL__

#include <Control.h>

#include "ModuleProxy.h"

class ColorLabel : public BControl {
public:
	ColorLabel(BRect frame, const char *name, const char *label,
		rgb_color initialColor, BMessage *message,
		uint32 rmask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
		uint32 flags = B_WILL_DRAW | B_NAVIGABLE); 
	
	rgb_color ValueAsColor() const;
	virtual void SetColor(rgb_color);

	virtual	void SetDivider(float);
	float Divider() const;

protected:
	virtual void AttachedToWindow();
	virtual void Draw(BRect);
	virtual void MessageReceived(BMessage *);

	virtual void MouseDown(BPoint);
	virtual status_t Invoke(BMessage *message = 0);

private:
	BRect ColorRect() const;

	rgb_color colorValue;
	float divider;

	ModuleProxy proxy;
	typedef BControl _inherited;
};

// some handy color-manipulation calls
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

#endif
