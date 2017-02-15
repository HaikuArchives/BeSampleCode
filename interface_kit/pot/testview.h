/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _testview_h
#define _testview_h

#include <View.h>
#include "colorutils.h"

class OffscreenView;

class TestView : public BView
{
public:
	TestView(BRect r, const char* name, uint32 resizingMode, uint32 flags);
	~TestView();
	
	void SetColor(rgb_color color) { m_color = color; }
	rgb_color Color() const { return m_color; }
	void SetBitmap(BBitmap* pBitmap);
	
	virtual void AttachedToWindow();
	virtual void Draw(BRect updateRect);
	virtual void FrameResized(float width, float height);	
	virtual void MessageReceived(BMessage* message);
	
	BBitmap* DrawOffscreen(BRect updateRect);
	void ChangeColor(uint8 val, color_channel ch);
	void ChangeColor(rgb_color color);
	void FixupScrollBars();
	
private:
	BBitmap* m_pBitmap;
	rgb_color m_color;
	OffscreenView* m_ov;
};

#endif /* _testview_h */
