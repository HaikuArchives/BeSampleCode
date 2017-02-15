/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <stdio.h>
#include <Bitmap.h>
#include <Message.h>
#include <ScrollBar.h>
#include <StopWatch.h>

#include "colorutils.h"
#include "constants.h"
#include "offscreenview.h"
#include "testview.h"

TestView::TestView(BRect r, const char* name, uint32 resizingMode, uint32 flags)
	: BView(r, name, resizingMode, flags), m_pBitmap(NULL), m_ov(NULL)
{
	SetViewColor(B_TRANSPARENT_32_BIT);
	ChangeColor(make_rgb_color(0,0,0,128));
}

TestView::~TestView()
{
	delete m_pBitmap;
	delete m_ov;
}

void TestView::SetBitmap(BBitmap* pBitmap)
{
	m_pBitmap = pBitmap;
	delete m_ov;
	m_ov = new OffscreenView(pBitmap->Bounds());
}

void TestView::AttachedToWindow()
{
	FixupScrollBars();
}

void TestView::Draw(BRect updateRect)
{
	BBitmap* pb = DrawOffscreen(updateRect);
	if (pb) {
		DrawBitmap(pb, updateRect, updateRect);
	}
}

void TestView::FrameResized(float /* width */, float /* height */)
{
	FixupScrollBars();
}

void TestView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case MSG_CHANGE_COLOR:
		{
			int32 val;
			int8 ch;
			if (msg->FindInt32("be:value", &val) == B_OK
				&& msg->FindInt8("channel", &ch) == B_OK)
			{
				ChangeColor((uint8)val, (color_channel) ch);
			}
			break;
		}
	default:
		BView::MessageReceived(msg);
		break;
	}
}

BBitmap* TestView::DrawOffscreen(BRect updateRect)
{
	if (! (m_ov && m_ov->Begin() == B_OK))
		return 0;
		
	m_ov->SetDrawingMode(B_OP_COPY);
	m_ov->DrawBitmap(m_pBitmap, updateRect, updateRect);
	m_ov->SetDrawingMode(B_OP_ALPHA);
	m_ov->SetHighColor(m_color);
	m_ov->FillRect(updateRect);
	return m_ov->End();
}

void TestView::ChangeColor(uint8 val, color_channel ch)
{
	rgb_color viewcolor = m_color;
	switch (ch) {
	case CHANNEL_RED:
		viewcolor.red = val;
		break;
	case CHANNEL_GREEN:
		viewcolor.green = val;
		break;
	case CHANNEL_BLUE:
		viewcolor.blue = val;
		break;
	case CHANNEL_ALPHA:
		viewcolor.alpha = val;
		break;
	}
	ChangeColor(viewcolor);
}

void TestView::ChangeColor(rgb_color color)
{
	m_color = color;
	Invalidate();
}

void TestView::FixupScrollBars()
{
	if (! m_pBitmap)
		return;

	BRect vBds = Bounds(), bBds = m_pBitmap->Bounds();
	float prop;
	float range;
	
	BScrollBar* sb = ScrollBar(B_HORIZONTAL);
	if (sb) {
		range = bBds.Width() - vBds.Width();
		if (range < 0) range = 0;
		prop = vBds.Width() / bBds.Width();
		if (prop > 1.0f) prop = 1.0f;
		sb->SetRange(0, range);
		sb->SetProportion(prop);
		sb->SetSteps(10, 100);
	}
	sb = ScrollBar(B_VERTICAL);
	if (sb) {
		range = bBds.Height() - vBds.Height();
		if (range < 0) range = 0;
		prop = vBds.Height() / bBds.Height();
		if (prop > 1.0f) prop = 1.0f;
		sb->SetRange(0, range);
		sb->SetProportion(prop);
		sb->SetSteps(10, 100);
	}
}
