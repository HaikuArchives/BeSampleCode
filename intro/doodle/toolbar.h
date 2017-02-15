/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _toolbar_h
#define _toolbar_h

/////////////////////////////////////////////////////////////////////////////
// Class: Toolbar
// --------------
// A class representing a toolbar, featuring BMenu-like access.
//
// MFC NOTE: The BeOS API does not provide a toolbar class. Here's how one
// could be written.

#include <List.h>
#include <Window.h>
#include "minivec.h"

class ToolbarItem;
class ScribbleDoc;

class Toolbar : public BWindow
{
// construction, destruction, operators
public:
	Toolbar(const char* name);

// overrides
public:
	virtual void Adopt(BWindow* pWin);
	virtual void Orphan(BWindow* pWin);
	virtual status_t UpdateUI();

// toolbar item access
public:
	BWindow* GetParentWindow();
	
	void AddItem(ToolbarItem* item);
	void AddSeparatorItem();
	int32 CountItems() const;

	ToolbarItem* FindItem(uint32 msgID) const;
		
	int32 IndexOf(ToolbarItem* item) const;
	ToolbarItem* ItemAt(int32 index) const;
	
	ToolbarItem* RemoveItem(int32 index);
	bool RemoveItem(ToolbarItem* item);

// implementation
private:
	void CalcLayout();

// data members
private:
	BWindow* m_pParent;	
	BView* m_bkgView;
	BList m_toolbarItems;
	MiniVec<int32> m_sepIndices;
};

#endif /* _toolbar_h */
