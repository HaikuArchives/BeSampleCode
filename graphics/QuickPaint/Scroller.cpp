/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Window.h>
#include <Shape.h>
#include "Scroller.h"
#include "PaintApp.h"

/*	Note that although the class is designed to be easily extensible to
	handle left-right scrolling as well as up-down scrolling, most of the left-right
	stuff isn't implemented.   So don't expect it to work. */

#define UPLEFT_ARROW_VISIBLE		0x00000001
#define UPLEFT_ARROW_DOWN			0x00000002
#define UPLEFT_PUSHED				0x00000004
#define DOWNRIGHT_ARROW_VISIBLE		0x00000008
#define DOWNRIGHT_ARROW_DOWN		0x00000010
#define DOWNRIGHT_PUSHED			0x00000020

BScroller::BScroller(const char *name, BView *scrolling)
	: BView(BRect(0,0,0,0),name,B_FOLLOW_NONE,
		B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_SUBPIXEL_PRECISE|B_FRAME_EVENTS)
{
	m_client = scrolling;
	m_clientExtent = 48; // These are hardcoded because I'm lazy
	m_stepSize = 48;
	BRect b = scrolling->Bounds();
	m_trackingState = 0;
	if (b.Width() >= b.Height()) {
		m_orientation = B_HORIZONTAL;
		b.InsetBy(-16,-4);
		SetResizingMode(B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP);
		scrolling->MoveTo(16,4);
	} else {
		m_orientation = B_VERTICAL;
		b.InsetBy(-4,-16);
		SetResizingMode(B_FOLLOW_TOP_BOTTOM|B_FOLLOW_LEFT);
		scrolling->MoveTo(4,16);
	};
	ResizeTo(b.Width(),b.Height());
	AddChild(scrolling);
	scrolling->SetResizingMode(B_FOLLOW_ALL);
	CheckArrows();
}

BScroller::~BScroller()
{
}

void BScroller::CheckArrows()
{
	uint32 old = m_trackingState;
	m_trackingState &= ~(UPLEFT_ARROW_VISIBLE|DOWNRIGHT_ARROW_VISIBLE);
	if (m_client->Bounds().top > 0) {
		if (!(old & UPLEFT_ARROW_VISIBLE)) Invalidate(UpLeftArrow());
		m_trackingState |= UPLEFT_ARROW_VISIBLE;
	} else {
		if (old & UPLEFT_ARROW_VISIBLE) Invalidate(UpLeftArrow());
		m_trackingState &= ~(UPLEFT_ARROW_DOWN|UPLEFT_PUSHED);
	};
	if (m_client->Bounds().bottom < m_clientExtent-1) {
		if (!(old & DOWNRIGHT_ARROW_VISIBLE)) Invalidate(DownRightArrow());
		m_trackingState |= DOWNRIGHT_ARROW_VISIBLE;
	} else {
		if (old & DOWNRIGHT_ARROW_VISIBLE) Invalidate(DownRightArrow());
		m_trackingState &= ~(DOWNRIGHT_ARROW_DOWN|DOWNRIGHT_PUSHED);
	};
}

void BScroller::FrameResized(float /* width */, float /* height */)
{
	CheckArrows();
}

BRect BScroller::UpLeftArrow()
{
	BRect b = Bounds();
	if (m_orientation == B_HORIZONTAL)	b.right = b.left+15;
	else								b.bottom = b.top+15;
	b.InsetBy(3,3);
	return b;
}

BRect BScroller::DownRightArrow()
{
	BRect b = Bounds();
	if (m_orientation == B_HORIZONTAL)	b.left = b.right-15;
	else								b.top = b.bottom-15;
	b.InsetBy(3,3);
	return b;
}

void BScroller::Draw(BRect /* updateRect */)
{
	BPoint tri[3];
	BRect r,b;
	float mid,height;
	rgb_color topColor,botColor;
	
	static rgb_color light = {240,240,240,255};
	// static rgb_color mezzolight = {190,190,190,255};
	static rgb_color mezzodark = {160,160,160,255};
	static rgb_color dark = {130,130,130,255};

	BeginLineArray(20);
	
	b = Bounds();
	AddLine(b.LeftTop(),b.RightTop(),light);
	AddLine(b.LeftTop(),b.LeftBottom(),light);
	AddLine(b.LeftBottom(),b.RightBottom(),dark);
	AddLine(b.RightBottom(),b.RightTop(),dark);

	b = m_client->Frame();
	b.InsetBy(-1,-1);
	AddLine(b.LeftTop(),b.RightTop(),dark);
	AddLine(b.LeftTop(),b.LeftBottom(),dark);
	AddLine(b.LeftBottom(),b.RightBottom(),light);
	AddLine(b.RightBottom(),b.RightTop(),light);

	if (m_trackingState & UPLEFT_ARROW_VISIBLE) {
		r = UpLeftArrow();
		mid = floor((r.left+r.right)/2+0.5);
		height = (r.bottom-r.top+1);
	
		tri[0].x = mid - height + 1;
		tri[1].x = mid + height - 1;
		tri[2].x = mid;
		tri[0].y =
		tri[1].y = r.bottom;
		tri[2].y = r.top;
	
		if (m_trackingState & UPLEFT_ARROW_DOWN) {
			BShape shape;
			shape.MoveTo(tri[0]);
			shape.LineTo(tri[2]);
			shape.LineTo(tri[1]);
			shape.Close();
		
			SetHighColor(mezzodark);
			MovePenTo(B_ORIGIN);
			FillShape(&shape);
			topColor = dark;
			botColor = light;
		} else {
			topColor = light;
			botColor = dark;
		};
	
		AddLine(tri[0],tri[1],light);
		AddLine(tri[0],tri[2],dark);
		AddLine(tri[2],tri[1],dark);
	
		tri[0].x += 2;
		tri[1].x -= 2;
		tri[2].y += 1;
		tri[0].y -= 1;
		tri[1].y -= 1;
		AddLine(tri[0],tri[1],botColor);
		AddLine(tri[0],tri[2],topColor);
		AddLine(tri[2],tri[1],topColor);
	};
	
	if (m_trackingState & DOWNRIGHT_ARROW_VISIBLE) {
		r = DownRightArrow();
		mid = floor((r.left+r.right)/2+0.5);
		height = (r.bottom-r.top+1);
	
		tri[0].x = mid - height + 1;
		tri[1].x = mid + height - 1;
		tri[2].x = mid;
		tri[0].y =
		tri[1].y = r.top;
		tri[2].y = r.bottom;

		if (m_trackingState & DOWNRIGHT_ARROW_DOWN) {
			BShape shape;
			shape.MoveTo(tri[0]);
			shape.LineTo(tri[2]);
			shape.LineTo(tri[1]);
			shape.Close();
		
			SetHighColor(mezzodark);
			MovePenTo(B_ORIGIN);
			FillShape(&shape);
			topColor = light;
			botColor = dark;
		} else {
			topColor = dark;
			botColor = light;
		};
	
		AddLine(tri[0],tri[1],dark);
		AddLine(tri[0],tri[2],light);
		AddLine(tri[2],tri[1],light);
	
		tri[0].x += 2;
		tri[1].x -= 2;
		tri[2].y -= 1;
		tri[0].y += 1;
		tri[1].y += 1;
		AddLine(tri[0],tri[1],botColor);
		AddLine(tri[0],tri[2],topColor);
		AddLine(tri[2],tri[1],topColor);
	};

	EndLineArray();
}

void BScroller::MouseDown(BPoint pt)
{
	if ((m_trackingState & UPLEFT_ARROW_VISIBLE) && UpLeftArrow().Contains(pt)) {
		m_trackingState |= UPLEFT_ARROW_DOWN|UPLEFT_PUSHED;
		Invalidate(UpLeftArrow());
		SetMouseEventMask(B_POINTER_EVENTS,B_LOCK_WINDOW_FOCUS);
		ScrollUpLeft();
	} else if ((m_trackingState & DOWNRIGHT_ARROW_VISIBLE) && DownRightArrow().Contains(pt)) {
		m_trackingState |= DOWNRIGHT_ARROW_DOWN|DOWNRIGHT_PUSHED;
		Invalidate(DownRightArrow());
		SetMouseEventMask(B_POINTER_EVENTS,B_LOCK_WINDOW_FOCUS);
		ScrollDownRight();
	} else return;
	
	SetFlags(Flags()|B_PULSE_NEEDED);
	Window()->SetPulseRate(100000);
}

void BScroller::MouseUp(BPoint /* point */)
{
	uint32 old = m_trackingState;
	m_trackingState &= ~(UPLEFT_ARROW_DOWN|UPLEFT_PUSHED|DOWNRIGHT_ARROW_DOWN|DOWNRIGHT_PUSHED);
	if (old & UPLEFT_PUSHED)
		Invalidate(UpLeftArrow());
	else if (old & DOWNRIGHT_PUSHED)
		Invalidate(DownRightArrow());
	else return;

	SetFlags(Flags()&~B_PULSE_NEEDED);
}

void BScroller::MouseMoved(BPoint pt, uint32 /* transit */, const BMessage * /* message */)
{
	if (m_trackingState & UPLEFT_PUSHED) {
		bool inUpLeft = UpLeftArrow().Contains(pt);
		if ((m_trackingState & UPLEFT_ARROW_DOWN) && !inUpLeft) {
			m_trackingState &= ~UPLEFT_ARROW_DOWN;
			Invalidate(UpLeftArrow());
		} else if (!(m_trackingState & UPLEFT_ARROW_DOWN) && inUpLeft) {
			m_trackingState |= UPLEFT_ARROW_DOWN;
			Invalidate(UpLeftArrow());
		};
	};
	if (m_trackingState & DOWNRIGHT_PUSHED) {
		bool inDownRight = DownRightArrow().Contains(pt);
		if ((m_trackingState & DOWNRIGHT_ARROW_DOWN) && !inDownRight) {
			m_trackingState &= ~DOWNRIGHT_ARROW_DOWN;
			Invalidate(DownRightArrow());
		} else if (!(m_trackingState & DOWNRIGHT_ARROW_DOWN) && inDownRight) {
			m_trackingState |= DOWNRIGHT_ARROW_DOWN;
			Invalidate(DownRightArrow());
		};
	};
}

void BScroller::MessageReceived(BMessage *msg)
{
	if (msg->what == bmsgExtents) {
		m_clientExtent = msg->FindInt32("y");
		CheckArrows();
	} else
		BView::MessageReceived(msg);
}

void BScroller::ScrollUpLeft()
{
	int32 y = (int32)m_client->Bounds().top;
	if (y < m_stepSize)
		m_client->ScrollBy(0,-y);
	else
		m_client->ScrollBy(0,-m_stepSize);
	CheckArrows();
}

void BScroller::ScrollDownRight()
{
	int32 y = (int32)m_client->Bounds().bottom;
	if ((y + m_stepSize) >= m_clientExtent)
		m_client->ScrollTo(0,m_clientExtent-m_client->Bounds().Height()-1);
	else
		m_client->ScrollBy(0,m_stepSize);
	CheckArrows();
}

void BScroller::Pulse()
{
	if (m_trackingState & UPLEFT_ARROW_DOWN) {
		ScrollUpLeft();
	} else if (m_trackingState & DOWNRIGHT_ARROW_DOWN) {
		ScrollDownRight();
	}
};
