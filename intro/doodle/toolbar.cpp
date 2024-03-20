/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "algobase.h"
#include <SupportDefs.h>
#include <stdio.h>
#include <MessageFilter.h>
#include <Rect.h>
#include "constants.h"
#include "dudedoc.h"
#include "toolbar.h"
#include "toolbaritem.h"

/////////////////////////////////////////////////////////////////////////////
// Class: ToolbarFilter
// --------------------
// A simple message filter which ignores mouse down messages if the toolbar
// has no parent.
//
// MFC NOTE: This is somewhat similar to "window subclassing," since
// we're tweaking a view's behavior by handling its messages first,
// and can be used to great advantage in similar types of situations.
class ToolbarFilter : public BMessageFilter
{
public:
	ToolbarFilter(Toolbar* pToolbar)
		: BMessageFilter(B_MOUSE_DOWN), m_pToolbar(pToolbar) { }
	filter_result Filter(BMessage* /* message */, BHandler** /* target */)
	{
		if (! m_pToolbar)
			return B_DISPATCH_MESSAGE;
		else 
			return (m_pToolbar->GetParentWindow())
				? B_DISPATCH_MESSAGE
				: B_SKIP_MESSAGE;
	}

private:
	Toolbar* m_pToolbar;
};



/////////////////////////////////////////////////////////////////////////////
// static variables

static const BRect toolbarMargin(5, 5, 5, 5);
static const float separatorMargin = 5;



/////////////////////////////////////////////////////////////////////////////
// Toolbar construction, destruction, operators

Toolbar::Toolbar(const char* name)
	: BWindow(BRect(0,0,20,20), name, B_FLOATING_WINDOW,
	B_NOT_RESIZABLE | B_NOT_CLOSABLE | B_NOT_ZOOMABLE
	| B_WILL_ACCEPT_FIRST_CLICK), m_pParent(0)
{
	// create background
	m_bkgView = new BView(Bounds(), "background", B_FOLLOW_ALL, B_WILL_DRAW);
	m_bkgView->SetViewColor(TOOLBAR_GREY);
	AddChild(m_bkgView);
}



/////////////////////////////////////////////////////////////////////////////
// Toolbar overrides

// Toolbar::Adopt
// --------------
// Indicates that a toolbar should identify itself with the
// specified window.
//
// MFC NOTE: In MFC, toolbars are associated with a
// frame window. Since there are no frame windows in BeOS,
// however, the toolbar must adjust to dispatch its messages
// to whatever the active window is.
void Toolbar::Adopt(BWindow* pWin)
{
	m_pParent = pWin;
}

// Toolbar::Orphan
// ---------------
// Indicates that a toolbar should cease to identify itself
// with the specified window.
//
// MFC NOTE: In MFC, toolbars are associated with a frame
// window. Since there are no frame windows in BeOS,
// however, the toolbar must become aware of when the
// window it refers to is no longer valid.
void Toolbar::Orphan(BWindow* pWin)
{
	if (m_pParent == pWin)
		m_pParent = 0;
}

// Toolbar::UpdateUI
// -----------------
// Updates the toolbar's UI to be in sync with the
// toolbar's parent window.
//
// MFC NOTE: This is similar to the UPDATE_COMMAND_UI
// mechanism.
status_t Toolbar::UpdateUI()
{
	if (! m_pParent)
		return B_ERROR;
		
	int32 i = 0;
	status_t res = B_OK;
	while (res == B_OK) {
		ToolbarItem* pItem = ItemAt(i++);
		if (! pItem)
			break;
		
		res = pItem->UpdateUI(m_pParent);
	}
	return res;
}

/////////////////////////////////////////////////////////////////////////////
// Toolbar item access
// -------------------
// These functions are designed to look like their BMenu counterparts.

BWindow* Toolbar::GetParentWindow()
{
	return m_pParent;
}

void Toolbar::AddItem(ToolbarItem* pItem)
{
	if (! pItem)
		return;

	pItem->AddFilter(new ToolbarFilter(this));
	m_toolbarItems.AddItem(pItem);
	m_bkgView->AddChild(pItem);
	CalcLayout();
}

// Important difference between this Toolbar implementation
// and the BMenu implementation: a toolbar separator is not
// a toolbar item and does not appear in the item list. A
// corollary to this is that the client cannot manipulate
// separator items once they are placed. A more complete
// implementation of Toolbar might change that, so that the
// user could access and move separator items more easily.
void Toolbar::AddSeparatorItem()
{
	if (! CountItems())
		return;
	
	int32 index = CountItems();
	int32 numSeps = m_sepIndices.CountItems();
	if (numSeps && (m_sepIndices[numSeps - 1] == index))
		return;
		
	// we maintain a list of indices at which separators
	// should be inserted.
	m_sepIndices.Push(index);
	CalcLayout();
}

int32 Toolbar::CountItems() const
{
	return m_toolbarItems.CountItems();
}

ToolbarItem* Toolbar::FindItem(uint32 msgID) const
{
	int32 i=0;
	ToolbarItem* pItem = ItemAt(i++);
	while (pItem) {
		BMessage* msg = pItem->Message();
		if (msg && (msg->what == msgID))
			break;
		pItem = ItemAt(i++);
	}
	return pItem;
}

int32 Toolbar::IndexOf(ToolbarItem* item) const
{
	return m_toolbarItems.IndexOf(item);
}

ToolbarItem* Toolbar::ItemAt(int32 index) const
{
	return static_cast<ToolbarItem*>(m_toolbarItems.ItemAt(index));
}

ToolbarItem* Toolbar::RemoveItem(int32 index)
{
	return static_cast<ToolbarItem*>(m_toolbarItems.RemoveItem(index));
}

bool Toolbar::RemoveItem(ToolbarItem* item)
{
	return m_toolbarItems.RemoveItem(item);
}



/////////////////////////////////////////////////////////////////////////////
// Toolbar implementation

// Toolbar::CalcLayout
// -------------------
// Calculates the location of all of the toolbar items within the toolbar.
void Toolbar::CalcLayout()
{
	int32 numItems = CountItems();
	int32 numSeps = m_sepIndices.CountItems();
	int32 curSep = (numSeps ? 0 : -1);
	float maxBottom = 0;
	float maxRight = 0;
	BPoint loc(toolbarMargin.left, toolbarMargin.top);
	for (int32 i=0; i<numItems; i++)
	{
		if ((curSep >= 0) && m_sepIndices[curSep] == i) {
			// insert a separator at this location
			loc.x += separatorMargin;
			curSep++;
		}
		// move the toolbar item to the next available space
		ToolbarItem* pItem = ItemAt(i);
		pItem->MoveTo(loc);
		BRect frame = pItem->Frame();
		maxRight = loc.x = frame.right + 1;
		maxBottom = max_c(maxBottom, frame.bottom);	
	}
	
	// resize the toolbar to fully encompass the layout + margin
	maxRight += toolbarMargin.right;
	maxBottom += toolbarMargin.bottom;
	ResizeTo(maxRight, maxBottom);
}
