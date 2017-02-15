/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <stdio.h>
#include <string.h>
#include <Bitmap.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Screen.h>
#include "constants.h"
#include "dudewin.h"
#include "toolbaritem.h"

/////////////////////////////////////////////////////////////////////////////
// static variables

static const rgb_color DARK_GREY = { 96, 96, 96 };
static const rgb_color SHADOW_GREY = { 127, 127, 127 };



/////////////////////////////////////////////////////////////////////////////
// ToolbarItem construction, destruction, operators

ToolbarItem::ToolbarItem(const char* name, BMessage* msg,
	BHandler* target)
	: BView(BRect(0,0,0,0), name, B_FOLLOW_LEFT|B_FOLLOW_TOP, B_WILL_DRAW),
	BInvoker(msg, target)
{
	SetViewColor(TOOLBAR_GREY);
}



/////////////////////////////////////////////////////////////////////////////
// ToolbarItem accessors

void ToolbarItem::SetMarked(bool marked)
{
	// only invalidate if state has changed
	if (m_bMarked != marked) {
		m_bMarked = marked;
		Invalidate();
	}
}

bool ToolbarItem::IsMarked() const
{
	return m_bMarked;
}



/////////////////////////////////////////////////////////////////////////////
// ToolbarItem overrides

// ToolbarItem::UpdateUI
// ---------------------
// Syncs the toolbar item to the window's contents
status_t ToolbarItem::UpdateUI(BWindow* parent)
{
	// try to lock window but forget it if we're about to
	// pulse again
	DudeWin* pWin = dynamic_cast<DudeWin*>(parent);
	if (! pWin)
		return B_ERROR;
	
	bigtime_t timeout = PULSE_EVENT_RATE - 2000;
	status_t res = pWin->LockWithTimeout(timeout);
	if (res != B_OK) {
		return res;
	}
	
	// find the menu item associated with this toolbar item
	// and set the toolbar item's state to the menu item's state	
	BMenuBar* menu = pWin->MenuBar();
	if (menu) {
		BMessage* msg = Message();
		if (msg) {
			BMenuItem* pItem = menu->FindItem(msg->what);
			if (pItem) {
				SetMarked(pItem->IsMarked());
			}
		}
	}
	pWin->Unlock();
	return B_OK;
}



/////////////////////////////////////////////////////////////////////////////
// ToolbarButton static variable definitions

const BPoint ToolbarButton::buttonMargin(2,2);



/////////////////////////////////////////////////////////////////////////////
// ToolbarButton construction, destruction, operators

ToolbarButton::ToolbarButton(const char* name, BBitmap* pBitmap,
	BMessage* msg, BHandler* target)
	: ToolbarItem(name, msg, target), m_pNormalBitmap(pBitmap)
{
	ResizeToPreferred();
}

ToolbarButton::~ToolbarButton()
{
	delete m_pNormalBitmap;
	if (m_pMarkedBitmap)
		delete m_pMarkedBitmap;
}



/////////////////////////////////////////////////////////////////////////////
// ToolbarButton overrides

void ToolbarButton::GetPreferredSize(float* width, float* height)
{
	// preferred size is bitmap + margin
	BRect bounds = m_pNormalBitmap->Bounds();
	*width = bounds.Width() + 2*buttonMargin.x;
	*height = bounds.Height() + 2*buttonMargin.y;
}

void ToolbarButton::ResizeToPreferred()
{
	float width, height;
	GetPreferredSize(&width, &height);
	ResizeTo(width, height);
}

void ToolbarButton::Draw(BRect /* updateRect */)
{
	DrawButtonFrame(IsMarked());
	DrawBitmap(IsMarked() ? m_pMarkedBitmap : m_pNormalBitmap,
		buttonMargin);
}

void ToolbarButton::MouseDown(BPoint pt)
{
	bool bInitiallyMarked = IsMarked();
	BRect bounds = Bounds();
	
	// track the mosue movement
	uint32 buttons;
	GetMouse(&pt, &buttons);
	while (buttons) {
		if (bounds.Contains(pt)) {
			// change state
			SetMarked(! bInitiallyMarked);
		} else {
			// revert state
			SetMarked(bInitiallyMarked);
		}
		snooze(50000);
		GetMouse(&pt, &buttons);
	}

	if (IsMarked() != bInitiallyMarked)
		// state changed
		Invoke();
}

void ToolbarButton::AttachedToWindow()
{
	BScreen screen(Window());
	CreateMarkedBitmap(screen);
}



/////////////////////////////////////////////////////////////////////////////
// ToolbarButton implementation

// ToolbarButton::CreateMarkedBitmap
// ---------------------------------
// Creates a bitmap representing the toolbar button
// in a marked (depressed) state, based on its normal
// bitmap and the screen's color map.
//
// MFC NOTE: Note that the 8-bit index mapping is based
// on the screen, and is unalterable under BeOS. Thus,
// there's no problem with applications fighting for
// palette space. Also note that BBitmap classes give
// you direct access to the bit buffer, which is stored
// in a device-independent way.
void ToolbarButton::CreateMarkedBitmap(BScreen& screen)
{
	m_pMarkedBitmap = new BBitmap(m_pNormalBitmap->Bounds(),
		m_pNormalBitmap->ColorSpace());
	
	if (m_pNormalBitmap->ColorSpace() != B_CMAP8) {
		// if the bitmap isn't 8-bit, make the marked bitmap
		// identical to the normal one. Handling other
		// bitmap sizes is left to the reader as an exercise.
		m_pMarkedBitmap->SetBits(m_pNormalBitmap->Bits(),
			m_pNormalBitmap->BitsLength(), 0,
			m_pNormalBitmap->ColorSpace());
		return;
	}

	// run through each pixel (8-bit value) in the normal bitmap.	
	uint8* oldBits = static_cast<uint8*>(m_pNormalBitmap->Bits());
	uint8* newBits = static_cast<uint8*>(m_pMarkedBitmap->Bits());
	int32 numPixels = m_pNormalBitmap->BitsLength();
	for (int32 i=0; i<numPixels; i++) {
		// find the RGB color which corresponds to this index
		rgb_color pix = screen.ColorForIndex(*(oldBits+i));
		// darken the color by 2/3
		pix.red = (uint8) (pix.red*2.0f/3);
		pix.green = (uint8) (pix.green*2.0f/3);
		pix.blue = (uint8) (pix.blue*2.0f/3);
		// find the closest index to this new color and
		// set the marked bitmap's pixel
		*(newBits+i) = screen.IndexForColor(pix);
	}
}

void ToolbarButton::DrawButtonFrame(bool pressed)
{
	BRect drawRect = Bounds();
	BPoint pt1, pt2;
	rgb_color topShadow, bottomShadow;
	topShadow = pressed ? DARK_GREY : PURE_WHITE;
	bottomShadow = pressed ? TOOLBAR_GREY : SHADOW_GREY;

	// stroke outline
	BeginLineArray(8);
	pt1.Set(drawRect.left, drawRect.top+1);
	pt2.Set(drawRect.left, drawRect.bottom-1);
	AddLine(pt1, pt2, PURE_BLACK);
	pt1.Set(drawRect.left+1, drawRect.top);
	pt2.Set(drawRect.right-1, drawRect.top);
	AddLine(pt1, pt2, PURE_BLACK);
	pt1.Set(drawRect.right, drawRect.top+1);
	pt2.Set(drawRect.right, drawRect.bottom-1);
	AddLine(pt1, pt2, PURE_BLACK);
	pt1.Set(drawRect.left+1, drawRect.bottom);
	pt2.Set(drawRect.right-1, drawRect.bottom);
	AddLine(pt1, pt2, PURE_BLACK);
	
	// stroke shadow
	drawRect.InsetBy(1,1);
	pt1.Set(drawRect.left, drawRect.top);
	pt2.Set(drawRect.left, drawRect.bottom-1);
	AddLine(pt1, pt2, topShadow);
	pt1.Set(drawRect.left+1, drawRect.top);
	pt2.Set(drawRect.right, drawRect.top);
	AddLine(pt1, pt2, topShadow);
	pt1.Set(drawRect.right, drawRect.top+1);
	pt2.Set(drawRect.right, drawRect.bottom);
	AddLine(pt1, pt2, bottomShadow);
	pt1.Set(drawRect.left, drawRect.bottom);
	pt2.Set(drawRect.right-1, drawRect.bottom);
	AddLine(pt1, pt2, bottomShadow);
	EndLineArray();	
}
