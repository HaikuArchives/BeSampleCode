/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Application.h>
#include <Bitmap.h>
#include <Locker.h>
#include "cmdtool.h"
#include "constants.h"
#include "icons.h"
#include "dudedoc.h"
#include "dudeview.h"
#include "dudewin.h"
#include "toolbaritem.h"

/////////////////////////////////////////////////////////////////////////////
// CmdToolbar static members

CmdToolbar* CmdToolbar::_toolbar = 0; // the singleton instance
BLocker CmdToolbar::_toolLock("CmdToolbar lock");

// CmdToolbar::Create
// ------------------
// Create the singleton instance if we haven't already done so.
void CmdToolbar::Create()
{
	if (! _toolLock.Lock())
		return;
		
	if (! _toolbar)
		_toolbar = new CmdToolbar();
		
	_toolLock.Unlock();
}

// CmdToolbar::Toolbar
// -------------------
// Returns the singleton instance, creating it if needed.
CmdToolbar* CmdToolbar::GetToolbar()
{
	CmdToolbar::Create();
	return _toolbar;
}

/////////////////////////////////////////////////////////////////////////////
// CmdToolbar static members

CmdToolbar::CmdToolbar()
	: Toolbar("Commands")
{
	InitObject();
}

CmdToolbar::~CmdToolbar()
{
	_toolbar = 0;
}

// CreateIconBitmap
// ----------------
// Assumes data is an array with the dimensions described in icons.h.
// Copies the data into a new BBitmap and returns the BBitmap object.
static BBitmap* CreateIconBitmap(const uint8* data)
{
	BBitmap* bitmap = new BBitmap(BRect(0,0,kBitmapWidth-1,kBitmapHeight-1),
		B_CMAP8);
	bitmap->SetBits(data, kBitmapBytes, 0, B_CMAP8);
	return bitmap;
}

void CmdToolbar::InitObject()
{
	AddItem(new ToolbarButton("New", CreateIconBitmap(kNewBitmap),
		new BMessage(MSG_FILE_NEW), be_app));
	AddItem(new ToolbarButton("Open", CreateIconBitmap(kOpenBitmap),
		new BMessage(MSG_FILE_OPEN), be_app));
	AddItem(new ToolbarButton("Save", CreateIconBitmap(kSaveBitmap),
		new BMessage(MSG_FILE_SAVE), 0));
	AddSeparatorItem();
	AddItem(new ToolbarButton("ThickOrThin", CreateIconBitmap(kWidthBitmap),
		new BMessage(MSG_PEN_THICK_OR_THIN), 0));
	AddSeparatorItem();
	AddItem(new ToolbarButton("Print", CreateIconBitmap(kPrintBitmap),
		new BMessage(MSG_FILE_PRINT), 0));
	AddItem(new ToolbarButton("About", CreateIconBitmap(kHelpBitmap),
		new BMessage(B_ABOUT_REQUESTED), be_app));
	MoveTo(100,100);
	Show();
}

void CmdToolbar::Adopt(BWindow* pWin)
{
	Toolbar::Adopt(pWin);
	DudeWin* pDudeWin = dynamic_cast<DudeWin*>(pWin);
	if (! pDudeWin)
		return;
		
	// retarget toolbar items to the new window/doc/view
	DudeView* targetView = pDudeWin->View();
	DudeDoc* pDoc = pDudeWin->Document();
	ToolbarItem* pItem;
	pItem = FindItem(MSG_FILE_SAVE);
	if (pItem) pItem->SetTarget(pDoc);
	pItem = FindItem(MSG_PEN_THICK_OR_THIN);
	if (pItem) pItem->SetTarget(pDoc);
	pItem = FindItem(MSG_FILE_PRINT);
	if (pItem) pItem->SetTarget(targetView);
}
