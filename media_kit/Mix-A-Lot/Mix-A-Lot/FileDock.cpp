/*
	
	FileDock.cpp
	
	Represents a "dock" view which accepts drag 'n' dropped files.

*/

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <stdio.h>
#include <Bitmap.h>
#include <NodeInfo.h>
#include <Debug.h>
#include "FileDock.h"

static const uint32 DRAG_FILE_MSG = 'dFIL';

static const rgb_color kFrameOutline = { 96, 96, 96, 255 };
static const rgb_color kFrameShadowDk = { 152, 152, 152, 255 };
static const rgb_color kFrameShadowLt = { 232, 232, 232, 255 };
static const BRect kLargeIconBounds(0,0,31,31);
static const color_space kIconColorSpace = B_CMAP8;
static const float kBorder = 2.0f;

FileDock::FileDock(BRect r, const char* name)
	: BView(r, name, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW),
	BInvoker()
{
	m_ref = 0;
	m_icon = 0;
	m_trackMouse = false;
	m_dragMsg = 0;
	m_highlight = false;
}

FileDock::~FileDock()
{
	delete m_ref;
	delete m_icon;
}

void FileDock::Draw(BRect updateRect)
{
	DrawFrame(updateRect);
	DrawIcon(updateRect);	
}

void FileDock::MouseDown(BPoint where)
{
	if (m_ref) {
		// There's something in the dock. Detach that something
		// and start a drag session.
		BRect iconFrame = IconFrame();
		if (iconFrame.Contains(where)) {
			BMessage dragMsg(DRAG_FILE_MSG);
			dragMsg.AddRef("ref", m_ref);
			BBitmap* dragBitmap = MakeDragBitmap();
			SetRef(0);
			where -= iconFrame.LeftTop();
			DragMessage(&dragMsg, dragBitmap, B_OP_ALPHA, where);
		}
	}
}

void FileDock::MouseMoved(BPoint /* where */, uint32 transit, const BMessage* dragMsg)
{
	if (transit == B_ENTERED_VIEW || transit == B_INSIDE_VIEW) {
		if (dragMsg && (dragMsg != m_dragMsg)) {
			// It's a new drag-n-drop message that we haven't
			// grokked yet.
			m_dragMsg = dragMsg;
			
			entry_ref ref;
			if (dragMsg->what == B_SIMPLE_DATA) {
				if (dragMsg->FindRef("refs", &ref) != B_OK)
					return;
			} else if (dragMsg->what == DRAG_FILE_MSG) {
				if (dragMsg->FindRef("ref", &ref) != B_OK)
					return;
			}
	
			if (IsValidRef(&ref)) {
				// It's a valid drag-n-drop file that we
				// understand, so highlight ourselves.
				m_highlight = true;
				Invalidate();
			}
		}
	} else {
		if (m_dragMsg) {
			// We were following a drag 'n' drop, but it left our view,
			// so we need to stop tracking and un-highlight ourselves.
			m_dragMsg = 0;
			if (m_highlight) {
				m_highlight = false;
				Invalidate();
			}
		}
	}
}

void FileDock::MouseUp(BPoint /* where */)
{
	if (m_dragMsg) {
		// We can stop handling the drag 'n' drop now. The dropped
		// item will appear in our message queue, to be handled in
		// MessageReceived().
		m_dragMsg = 0;
		if (m_highlight) {
			m_highlight = false;
			Invalidate();
		}
	}
}

void FileDock::MessageReceived(BMessage* message)
{
	if (message->WasDropped()) {
		// A drag 'n' drop message!
		HandleDrop(message);
		return;
	}
	
	BView::MessageReceived(message);		
}

void FileDock::AttachedToWindow()
{
	// We use our parent's view color to draw our background.
	BView* p = Parent();
	if (p) {
		SetViewColor(p->ViewColor());
	}
}

void FileDock::GetPreferredSize(float* width, float* height)
{
	// Our preferred size is just large enough to draw
	// a large icon, plus the border on each side.
	*width = kLargeIconBounds.Width() + 2*kBorder;
	*height = kLargeIconBounds.Height() + 2*kBorder;
}

void FileDock::ResizeToPreferred()
{
	float width, height;
	GetPreferredSize(&width, &height);
	ResizeTo(width, height);
}

status_t FileDock::Invoke(BMessage* message)
{
	// Find out if we're using a passed-in message or
	// our default message.
	if (! message)
		message = Message();
	if (! message)
		return B_BAD_VALUE;
	
	// Create the message we're going to send, and
	// add our special information to it.
	BMessage sendMsg(*message);
	sendMsg.AddInt64("when", system_time());
	sendMsg.AddPointer("source", this);
	if (m_ref)
		sendMsg.AddRef("refs", m_ref);
	return BInvoker::Invoke(&sendMsg);
}

BRect FileDock::IconFrame() const
{
	// This calculates the frame of the icon, which is the size
	// of a large icon, and centered in the view.
	BRect vb = Bounds();
	BPoint center((vb.left+vb.right)/2, (vb.top+vb.bottom)/2);
	BRect iconFrame = kLargeIconBounds;
	BPoint iconLoc(center.x - kLargeIconBounds.Width()/2, center.y - kLargeIconBounds.Height()/2);
	iconFrame.OffsetBy(iconLoc);
	return iconFrame;
}

void FileDock::DrawFrame(BRect /* updateRect */)
{
	// This draws the 3D border around the dock icon.
	BRect r = Bounds();
	
	if (m_highlight) {
		// we're now handling a drag & drop
		SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
	} else {
		SetHighColor(kFrameOutline);
	}
	StrokeRect(r);

	r.InsetBy(1,1);
	BeginLineArray(4);
	BPoint p1, p2;
	p1.Set(r.left+1, r.top);
	p2.Set(r.right, r.top);
	AddLine(p1, p2, kFrameShadowDk);
	p1.Set(r.right, r.top+1);
	p2.Set(r.right, r.bottom);
	AddLine(p1, p2, kFrameShadowLt);
	p1.Set(r.right-1, r.bottom);
	p2.Set(r.left, r.bottom);
	AddLine(p1, p2, kFrameShadowLt);
	p1.Set(r.left, r.bottom-1);
	p2.Set(r.left, r.top);
	AddLine(p1, p2, kFrameShadowDk);
	EndLineArray();
}

void FileDock::DrawIcon(BRect /* updateRect */)
{
	// This draws the icon. B_OP_OVER does the right thing with
	// fully transparent pixels in the bitmap.
	if (! m_icon)
		return;

	BRect iconFrame = IconFrame();	
	SetDrawingMode(B_OP_OVER);
	DrawBitmap(m_icon, iconFrame.LeftTop());
}

void FileDock::SetRef(const entry_ref* ref)
{
	// Set up the file dock to point to a particular entry.
	delete m_ref;
	if (ref) {
		m_ref = new entry_ref(*ref);
	} else {
		m_ref = 0;
	}
	SetIcon(m_ref);
	Invoke();
}

void FileDock::SetIcon(const entry_ref* ref)
{
	delete m_icon;
	if (ref) {
		// Load the large icon that Tracker uses to display this entry.
		m_icon = new BBitmap(kLargeIconBounds, kIconColorSpace);
		entry_ref tref(*ref);
		status_t err = BNodeInfo::GetTrackerIcon(&tref, m_icon);
		if (err != B_OK) {
			printf("couldn't load icon\n");
			delete m_icon;
		}
	} else {
		m_icon = 0;
	}
	Invalidate();
}

void FileDock::HandleDrop(BMessage* message)
{
	entry_ref ref;
	if (message->what == B_SIMPLE_DATA) {
		if (message->FindRef("refs", &ref) != B_OK)
			return;
	} else if (message->what == DRAG_FILE_MSG) {
		if (message->FindRef("ref", &ref) != B_OK)
			return;
	}
	
	if (IsValidRef(&ref)) {
		SetRef(&ref);
	}
}

// Once we receive a drop, this makes the final
// judgement as to whether the ref is a valid one
// for our kind of dock. Override this if you only
// want to allow certain kinds of entries in your
// dock.
bool FileDock::IsValidRef(const entry_ref* /* ref */)
{
	return true;
}

BBitmap* FileDock::MakeDragBitmap()
{
	// This is the authorized way of making a translucent bitmap.
	// The Tracker does something very similar to this.
	ASSERT(m_icon);
	BBitmap* dest = new BBitmap(m_icon->Bounds(), m_icon->ColorSpace(), true);
	if (dest->Lock()) {
		BView* view = new BView(dest->Bounds(), "", B_FOLLOW_NONE, 0);
		dest->AddChild(view);
		view->SetHighColor(B_TRANSPARENT_COLOR);
		view->FillRect(view->Bounds());
		// Make the bitmap drawing half-transparent, and use composite
		// alpha blending to calculate the transparencies correctly.
		view->SetHighColor(0,0,0,128);
		view->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE);
		view->SetDrawingMode(B_OP_ALPHA);
		view->DrawBitmap(m_icon);
		view->Sync();
		dest->RemoveChild(view);
		dest->Unlock();
	}
	return dest;
}
