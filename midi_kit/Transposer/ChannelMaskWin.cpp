// ChannelMaskWin.cpp
// ------------------
// Implements the ChannelMaskWin class.
//
// Copyright 1999, Be Incorporated.   All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#include <CheckBox.h>
#include <Message.h>
#include <Messenger.h>
#include <String.h>
#include "ChannelMaskWin.h"
#include "TransposerApp.h"

const uint32 MSG_CHANNEL_MASK_CHANGED = 'Gmsk';
const float ROW_TOP = 10.0f;
const float COLUMN_LEFT = 10.0f;
const float COLUMN_WIDTH = 40.0f;
const float ROW_HEIGHT = 30.0f;
const float BOX_WIDTH = 30.0f;
const float BOX_HEIGHT = 20.0f;

ChannelMaskWin::ChannelMaskWin(uint16 startMask)
	: BWindow(BRect(100, 100, 270, 230), "Edit Channel Mask", B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	BView* bkg = new BView(Bounds(), "Background", B_FOLLOW_ALL, B_WILL_DRAW);
	bkg->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(bkg);
	for (uint8 i = 0; i<4; i++) {
		for (uint8 j=0; j<4; j++) {
			uint8 ch = i*4 + j;
			BString name;
			name << ch + 1;
			BRect r;
			r.left = COLUMN_LEFT + i*COLUMN_WIDTH;
			r.top = ROW_TOP + j*ROW_HEIGHT;
			r.right = r.left + BOX_WIDTH;
			r.bottom = r.top + BOX_HEIGHT;
			BCheckBox* box = new BCheckBox(r, name.String(), name.String(),
				new BMessage(MSG_CHANNEL_MASK_CHANGED));
			box->SetValue((startMask >> ch) & 0x1);
			m_boxes.AddItem(box);
			bkg->AddChild(box);		
		}
	}
}

bool ChannelMaskWin::QuitRequested()
{
	Hide();
	return false;
}

void ChannelMaskWin::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case MSG_CHANNEL_MASK_CHANGED:
		{
			uint16 mask = 0x0000;
			for (uint8 i = 0; i < 16; i++) {
				// channel 1 should be LSB, so fill in the channels
				// backwards, from 16 to 1.
				mask <<= 1;
				BCheckBox* box = (BCheckBox*) m_boxes.ItemAt(15 - i);
				if (box) {
					mask |= box->Value();
				}
			}

			BMessage applyMsg(MSG_APPLY_CHANNEL_MASK);
			applyMsg.AddInt16("mask", mask);
			BMessenger msgr(be_app);
			if (msgr.SendMessage(&applyMsg, (BHandler*) NULL, 100000LL) != B_OK) {
				BCheckBox* box = NULL;
				if (msg->FindPointer("source", (void**) &box) == B_OK) {
					box->SetValue(! box->Value());
				}
			}
		}
		break;
	default:
		break;
	}
}
