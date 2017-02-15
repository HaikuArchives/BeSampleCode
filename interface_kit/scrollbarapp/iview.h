/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef __iview_h
#define __iview_h 1

#include <Bitmap.h>
#include <View.h>
#include <Point.h>

enum { SCALE_NORMAL = 0, SCALE_TOFIT, SCALE_MAX, SCALE_LAST };

class TImageView : public BView {
public:
	TImageView(BRect frame, const char *name, float nscale, int32 nscale_mode);
	virtual void Draw(BRect invalid);
	virtual void AttachedToWindow(void);
	virtual void FrameResized(float width, float height);
	void GetMaxSize(float *maxx, float *maxy);
	void SetImageScale(float nscale, int32 nscale_mode);
private:
	BBitmap *image;
	float scale;
	int32 scale_mode;
	BPoint skip;
	void FixupScrollbars(void);
};

#endif // __iview_h

