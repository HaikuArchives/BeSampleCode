// TransposerWin.cpp
// -----------------
// Implements the Transposer main window class.
//
// Copyright 1999, Be Incorporated.   All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#include <MenuBar.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Slider.h>

#include "TransposerApp.h"
#include "TransposerWin.h"

TransposerWin::TransposerWin(const char* name, int8 initTranspose)
		: BWindow(BRect(50,50,250,140), name, B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	BMenuBar* bar = new BMenuBar(BRect(0,0,1,1), "Menu Bar");
	BMenu* menu = new BMenu("Transpose");
	BMenuItem*item = new BMenuItem("Channel Mask...", new BMessage(MSG_EDIT_CHANNEL_MASK));
	item->SetTarget(be_app);
	menu->AddItem(item);
	menu->AddSeparatorItem();
	item = new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q');
	item->SetTarget(be_app);
	menu->AddItem(item);
	bar->AddItem(menu);
	AddChild(bar);
	
	BRect r = Bounds();
	r.top = bar->Frame().bottom + 1;
	BView* pView = new BView(r, "Bkg View", B_FOLLOW_ALL, B_WILL_DRAW);
	pView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(pView);
	BSlider* pSlider = new BSlider(BRect(10, 10, 190, 30), "Transpose",
		"Transpose", new BMessage(MSG_CHANGE_TRANSPOSE), -36, 36);
	pSlider->SetValue(initTranspose);
	pSlider->SetLimitLabels("-36", "+36");
	pSlider->SetHashMarks(B_HASH_MARKS_BOTH);
	pSlider->SetHashMarkCount(7);
	pView->AddChild(pSlider); 
	pSlider->SetTarget(be_app); 
	pSlider->MakeFocus();
}

void TransposerWin::Quit()
{
	be_app->PostMessage(MSG_WINDOW_CLOSED);
	BWindow::Quit();
}
