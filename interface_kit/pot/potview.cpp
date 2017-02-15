/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <stdio.h>
#include <CheckBox.h>
#include <Message.h>

#include "constants.h"
#include "potview.h"
#include "pot.h"
#include "testview.h"
#include "testwin.h"

PotView::PotView(BRect frame)
	: BView(frame, "PotView", B_FOLLOW_ALL, B_WILL_DRAW)
{
	SetViewColor(216,216,216);
}

void PotView::AttachedToWindow()
{
	BRect r;
	
	r.Set(10, 10, 100, 110);
	AddPotAndBox(r, CHANNEL_RED, make_rgb_color(255, 30, 30),
		&m_red, "red");
	
	r.OffsetBy(100, 0);
	AddPotAndBox(r, CHANNEL_GREEN, make_rgb_color(30, 255, 30),
		&m_green, "green");
	
	r.OffsetBy(100, 0);
	AddPotAndBox(r, CHANNEL_BLUE, make_rgb_color(30, 30, 255),
		&m_blue, "blue");

	r.OffsetBy(100, 0);
	AddPotAndBox(r, CHANNEL_ALPHA, make_rgb_color(100, 100, 100),
		&m_alpha, "alpha");	
	
	// Use SetEventMask to dispatch command-click and drags.
	// For some reason, MouseDown events over this view's
	// children don't get broadcasted here; it may be an
	// app_server bug. Watch this space...
	SetEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
}

void PotView::MouseDown(BPoint where)
{
	InformMouseDown(m_red, where);
	InformMouseDown(m_green, where);
	InformMouseDown(m_blue, where);
	InformMouseDown(m_alpha, where);	
}

void PotView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case MSG_CHANGE_FOCUS:
		{
			void* ptr;
			if (msg->FindPointer("new_target", &ptr) == B_OK) {
				if (ptr) {
					TestWin* pWin = static_cast<TestWin*>(ptr);
					TestView* pView = pWin->GetTestView();
					m_red->SetTarget(pView);
					m_green->SetTarget(pView);
					m_blue->SetTarget(pView);
					m_alpha->SetTarget(pView);
				}
			}
			rgb_color color;
			if (msg->FindInt32("new_color", (int32*) &color) == B_OK) {
				m_red->SetValue(color.red);
				m_green->SetValue(color.green);
				m_blue->SetValue(color.blue);
				m_alpha->SetValue(color.alpha);
			}
		}
	default:
		BView::MessageReceived(msg);
		break;
	}
}

void PotView::AddPotAndBox(BRect r, color_channel ch,
	rgb_color color, BPot** hPot, const char* name)
{
	BMessage* msg;
	BCheckBox* pBox;
	BPot* pPot;
	
	msg = new BMessage(MSG_CHANGE_COLOR);
	msg->AddInt8("channel", ch);	
	*hPot = new BPot(r, name, "", msg, 0, 255, B_FOLLOW_NONE, B_WILL_DRAW);
	pPot = *hPot;
	pPot->SetLimitLabels("0", "255");
	pPot->SetHighColor(color);
	AddChild(pPot);
	
	msg = new BMessage(MSG_CAPTURE_MOUSE);
	pBox = new BCheckBox(BRect(r.left, r.bottom+5, r.left, r.bottom+5),
		"", "", msg);
	pBox->ResizeToPreferred();
	pBox->SetTarget(pPot);
	AddChild(pBox);
}

void PotView::InformMouseDown(BPot* pot, BPoint where)
{
	uint32 buttons;
	GetMouse(&where, &buttons);
	if (buttons & B_PRIMARY_MOUSE_BUTTON) {
		key_info myInfo;
		uint32 modifiers = 0;
		if (get_key_info(&myInfo) == B_OK) {
			modifiers = myInfo.modifiers;
		}
		
		if (pot->IsMarked() && (modifiers & B_COMMAND_KEY)) {
			pot->MouseDown(where);
		}
	}
}
