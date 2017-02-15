/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _offscreenview_h
#define _offscreenview_h

#include <View.h>

class OffscreenView : public BView
{
public:
	OffscreenView(BRect r);
	~OffscreenView();
	
	status_t Begin();
	BBitmap* End();

	BBitmap* Bitmap() const { return m_pb; }

	void FrameResized(float width, float height);
	
private:
	BBitmap* m_pb;
};

#endif /* _offscreenview_h */
