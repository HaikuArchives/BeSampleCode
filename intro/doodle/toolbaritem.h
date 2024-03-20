/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _toolbaritem_h
#define _toolbaritem_h

/////////////////////////////////////////////////////////////////////////////
// Class: ToolbarItem
// ------------------
// A class representing an item in a toolbar, featuring BMenuItem-like
// access. Currently, it only allows you to mark/unmark it; it does not
// yet support enabled/disabled.
//
// MFC NOTE: The BeOS API does not provide a toolbar class. Here's how one
// could be written.

#include <Invoker.h>
#include <View.h>
#include <Screen.h>

class ToolbarItem : public BView, public BInvoker
{
// construction, destruction, operators
public:
	ToolbarItem(const char* name, BMessage* msg, BHandler* target);

// overrides
public:
	virtual status_t UpdateUI(BWindow* parent);

// accessors
public:
	void SetMarked(bool marked);
	bool IsMarked() const;
	
// data members
private:
	bool m_bMarked;
};

/////////////////////////////////////////////////////////////////////////////
// Class: ToolbarButton
// --------------------
// A toolbar button which displays a bitmap of any size.

class ToolbarButton : public ToolbarItem
{
// construction, destruction, operators
public:
	ToolbarButton(const char* name, BBitmap* icon, BMessage* msg,
		BHandler* target);
	virtual ~ToolbarButton();
	
// overrides
public:
	virtual void GetPreferredSize(float* width, float* height);
	virtual void ResizeToPreferred();
	virtual void Draw(BRect updateRect);
	virtual void MouseDown(BPoint pt);
	virtual void AttachedToWindow();

// implementation		
private:
	void CreateMarkedBitmap(BScreen& screen);
	void DrawButtonFrame(bool pressed);

// data members
private:
	static const BPoint buttonMargin;
	BBitmap* m_pNormalBitmap;
	BBitmap* m_pMarkedBitmap;
};

#endif /* _toolbaritem_h */
