//******************************************************************************
//
//	File:			SendPanel.cpp
//
//	Written by:		Douglas Wright
//
//******************************************************************************

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <stdio.h>
#include <Box.h>
#include <Button.h>
#include <Menu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <TextControl.h>
#include <TextView.h>

#include "PropDump.h"
#include "SendPanel.h"

const BRect kPanelRect(120, 100, 550, 400);

const int32 SEND_MSG = 'send';
const int32 ADD_MSG	= 'addm';
const int32 REMOVE_MSG = 'remm';
const int32 POP_UP = 'popu';

const bigtime_t HUGE_TIMEOUT = 1000000;

/* Send Panel */
SendPanel::SendPanel(BMessenger *msgr)
	:BWindow(kPanelRect, "Send", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	SendView *view = new SendView(Bounds());
	view->SetMessenger(msgr);
	AddChild(view);
}

SendPanel::~SendPanel()
{
}

void 
SendPanel::MessageReceived(BMessage *msg)
{
	BWindow::MessageReceived(msg);
}

/* Send View */
SendView::SendView(BRect rect)
	:BView(rect, "SendView", B_FOLLOW_ALL, B_WILL_DRAW)
{
}


SendView::~SendView()
{
}

void 
SendView::AttachedToWindow()
{
	SetViewColor(200,200,200,255);
	SetLowColor(200,200,200,255);
	
	BRect bounds(Bounds());
	bounds.InsetBy(15, 30);
	bounds.bottom -= 190;
	BBox* pBox = new BBox(bounds, "Box");
	pBox->SetLabel("Specifiers");
	AddChild(pBox);

	SetUpButtons(pBox);

	// add popup for message types
	fMessageTypeMenu = new BMenu("MessageTypeMenu");
	fMessageTypeMenu->AddItem(new BMenuItem("B_GET_PROPERTY", new BMessage(B_GET_PROPERTY)));
	fMessageTypeMenu->AddItem(new BMenuItem("B_SET_PROPERTY", new BMessage(B_SET_PROPERTY)));
	fMessageTypeMenu->AddItem(new BMenuItem("B_CREATE_PROPERTY", new BMessage(B_CREATE_PROPERTY)));
	fMessageTypeMenu->AddItem(new BMenuItem("B_DELETE_PROPERTY", new BMessage(B_DELETE_PROPERTY)));
	fMessageTypeMenu->AddItem(new BMenuItem("B_COUNT_PROPERTIES", new BMessage(B_COUNT_PROPERTIES)));
	fMessageTypeMenu->AddItem(new BMenuItem("B_EXECUTE_PROPERTY", new BMessage(B_EXECUTE_PROPERTY)));
	fMessageTypeMenu->AddItem(new BMenuItem("B_GET_SUPPORTED_SUITES", new BMessage(B_GET_SUPPORTED_SUITES)));
	fMessageTypeMenu->ItemAt(0)->SetMarked(true);
	fMessageTypeMenu->SetLabelFromMarked(true);
	
	BRect rect(bounds);
	rect.top -= 25;
	rect.right = rect.left + 200;
	rect.bottom = rect.top + 15;
	BMenuField* menuField = new BMenuField(rect, "MenuField", "Message Type:", fMessageTypeMenu);
	menuField->SetDivider(75.0);
	AddChild(menuField);

	// add Search button
	rect = Bounds();
	rect.left = rect.right - 85;
	rect.top = rect.top + 87;
	rect.right = rect.left + 60;
	rect.bottom = rect.top + 20;
	BButton* button = new BButton(rect, "send", "Send",
		new BMessage(SEND_MSG), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	AddChild(button);
	button->SetTarget(this);

	// add Text Interface
	BRect r(Bounds());
	r.InsetBy(15, 10);
	r.right -= 15;
	r.top += 110;
	BRect f(r);
	f.OffsetTo(B_ORIGIN);
	f.left += 2;
	f.top += 2;
	fText = new BTextView(r, "textview", f, 
										B_FOLLOW_ALL, 
         								B_WILL_DRAW);
	fText->MakeEditable(false);
	BScrollView *scview = new BScrollView("scroller", fText, 
         								B_FOLLOW_BOTTOM | B_FOLLOW_LEFT_RIGHT, 
         								0, false, true, B_FANCY_BORDER);
    AddChild(scview);

}

void 
SendView::MessageReceived(BMessage *msg)
{
	switch(msg->what){
		case ADD_MSG:
			AddSpecView();
			break;
		case REMOVE_MSG:
			RemoveSpecView();
			break;
		case SEND_MSG:{
			DoSendMessage();
			break;
		}
		default:
			BView::MessageReceived(msg);
	}
}

void
SendView::DoSendMessage()
{	
	BString str;
	BMessage reply;			
	BMessage script_msg;
	BuildMessage(&script_msg);

	fText->Delete(0, fText->TextLength());

	str += "Message sent: \n";
	str += DumpMessage(&script_msg, MSG_IS_CMD);
	str += "\n\n";
	fText->Insert(fText->TextLength(), str.String(), str.Length());
	status_t result = fMessenger->SendMessage(&script_msg, &reply, HUGE_TIMEOUT, HUGE_TIMEOUT);
	if(result == B_OK){
		str = "Reply received: \n";
		str += DumpMessage(&reply, MSG_IS_CMD);
		str += "\n\n";
		fText->Insert(fText->TextLength(), str.String(), str.Length());
	}
}

void 
SendView::BuildMessage(BMessage *msg) const
{
	BMenuItem *item = fMessageTypeMenu->FindMarked();
	msg->what = item->Command();
	int32 count = specViewList.CountItems();
	for(int i=count-1; i>=0; --i){
		SpecView *spec = (SpecView *)specViewList.ItemAt(i);
		switch(spec->fMenu->Menu()->FindMarked()->Command()){
			case B_DIRECT_SPECIFIER:
				msg->AddSpecifier(spec->fText1->Text());
				break;
			case B_NAME_SPECIFIER:
				msg->AddSpecifier(spec->fText1->Text(), spec->fText2->Text());
				break;
			case B_INDEX_SPECIFIER:
				{
					int32 index;
					if (GetTCInt32(spec->fText2, &index) == B_OK) {
						msg->AddSpecifier(spec->fText1->Text(), index);
					} else {
						printf("can't build msg: %s is not a valid index specifier!\n",
							spec->fText2->Text());
						return;
					}
					break;
				}
			case B_REVERSE_INDEX_SPECIFIER:
				{
					int32 index;
					if (GetTCInt32(spec->fText2, &index) == B_OK) {
						BMessage msgSpec(B_REVERSE_INDEX_SPECIFIER);
						msgSpec.AddString("property", spec->fText1->Text());
						msgSpec.AddInt32("index", index);
						msg->AddSpecifier(&msgSpec);
					} else {
						printf("can't build msg: %s is not a valid index specifier!\n",
							spec->fText2->Text());
						return;
					}
					break;
				}
			case B_RANGE_SPECIFIER:
				{
					int32 begin, end;
					if (GetTCInt32Range(spec->fText2, &begin, &end) == B_OK) {
						BMessage msgSpec(B_RANGE_SPECIFIER);
						msgSpec.AddString("property", spec->fText1->Text());
						msgSpec.AddInt32("index", begin);
						msgSpec.AddInt32("range", end);
						msg->AddSpecifier(&msgSpec);
					} else {
						printf("can't build msg: %s is not a valid range specifier!\n",
							spec->fText2->Text());
						return;
					}
					break;
				}
			case B_REVERSE_RANGE_SPECIFIER:
				{
					int32 begin, end;
					if (GetTCInt32Range(spec->fText2, &begin, &end) == B_OK) {
						BMessage msgSpec(B_REVERSE_RANGE_SPECIFIER);
						msgSpec.AddString("property", spec->fText1->Text());
						msgSpec.AddInt32("index", begin);
						msgSpec.AddInt32("range", end);
						msg->AddSpecifier(&msgSpec);
					} else {
						printf("can't build msg: %s is not a valid range specifier!\n",
							spec->fText2->Text());
						return;
					}
					break;
				}
			case B_ID_SPECIFIER:
				{
					uint32 id;
					if (GetTCUint32(spec->fText2, &id) == B_OK) {
						BMessage msgSpec(B_ID_SPECIFIER);
						msgSpec.AddString("property", spec->fText1->Text());
						msgSpec.AddInt32("id", id);
						msg->AddSpecifier(&msgSpec);
					} else {
						printf("can't build msg: %s is not a valid ID specifier!\n",
							spec->fText2->Text());
						return;
					}
					break;
				}						
			default:
				break;
		}
	}
}


status_t
SendView::GetTCInt32(const BTextControl* pCtrl, int32* dest) const
{
	const char* buffer = pCtrl->Text();
	char guard[2];
	int32 bufInt;
	int res = sscanf(buffer, "%ld%1s", &bufInt, guard);
	if (res != 1) {
		return B_BAD_VALUE;
	} else {
		*dest = bufInt;
		return B_OK;
	}
}

status_t
SendView::GetTCUint32(const BTextControl* pCtrl, uint32* dest) const
{
	const char* buffer = pCtrl->Text();
	const char* conversion;
	char guard[2];
	uint32 bufInt;
	if (! strncmp(buffer, "0x", 2)) {
		conversion = "0x%lx%1s";
	} else if (! strncmp(buffer, "0X", 2)) {
		conversion = "0X%lx%1s";
	} else if (*buffer == '0') {
		conversion = "0%lo%ls";
	} else {
		conversion = "%lu%ls";
	}	
	int res = sscanf(buffer, conversion, &bufInt, guard);
	if (res != 1) {
		return B_BAD_VALUE;
	} else {
		*dest = bufInt;
		return B_OK;
	}
}

status_t
SendView::GetTCInt32Range(const BTextControl* pCtrl, int32* begin, int32* end) const
{
	const char* buffer = pCtrl->Text();
	char guard[2];
	int32 bufBegin, bufEnd;
	int res = sscanf(buffer, "%ld - %ld%1s", &bufBegin, &bufEnd, guard);
	if (res != 2) {
		return B_BAD_VALUE;
	} else {
		*begin = bufBegin;
		*end = bufEnd;
		return B_OK;
	}
}

void 
SendView::SetMessenger(BMessenger *msgr)
{
	fMessenger = msgr;
}

void 
SendView::AddSpecView()
{
	BBox *box = dynamic_cast<BBox *>(FindView("Box"));
	BRect bounds(Bounds());

	SpecView *previous = (SpecView *)specViewList.LastItem();
	
	Window()->ResizeBy(0, 30);

	bounds = Bounds();
	bounds.InsetBy(15, 30);
	bounds.bottom -= 190;

	box->ResizeTo(bounds.Width(), bounds.Height());
	if (previous) {
		bounds = previous->Frame();
		bounds.OffsetBy(0, 30);
	} else {
		bounds = box->Bounds();
		bounds.InsetBy(5, 5);
		bounds.top += 10;
		bounds.bottom = bounds.top + 20;
	}

	SpecView *specView = new SpecView(bounds);
	specViewList.AddItem(specView);
	
	box->AddChild(specView);

	SetUpButtons(box);
}

void 
SendView::RemoveSpecView()
{
	if (specViewList.CountItems() < 1)
		return;

	BBox *box = (BBox*)FindView("Box");
	SpecView *specView = (SpecView *)specViewList.LastItem();
	if (!box || !specView)
		return;
	
	Window()->ResizeBy(0, -30);
	BRect bounds(Bounds());
	bounds.InsetBy(15, 30);
	bounds.bottom -= 190;
	box->ResizeTo(bounds.Width(), bounds.Height());

	specViewList.RemoveItem(specView);
	specView->RemoveSelf();
	delete specView;

	specView = (SpecView *)specViewList.ItemAt(specViewList.CountItems() - 1);

	if (specViewList.CountItems() != 0)
		return;
		
	BButton *button = (BButton*)Window()->FindView("remove");
	if (button)
		button->SetEnabled(false);

}

void 
SendView::SetUpButtons(BBox *box)
{
	BButton *button = (BButton*)Window()->FindView("remove");
	if (!button) {	
		BRect rect = box->Bounds();
		rect.InsetBy(5, 10);
		rect.top = rect.bottom - 20;
		rect.right = rect.left + 40;
		
		button = new BButton(rect, "add", "Add", new BMessage(ADD_MSG),
			B_FOLLOW_RIGHT + B_FOLLOW_BOTTOM);
		button->SetTarget(this);
		box->AddChild(button);
	
		rect.OffsetBy(50, 0);
		rect.right = rect.left + 55;
		button = new BButton(rect, "remove", "Remove", new BMessage(REMOVE_MSG),
			B_FOLLOW_RIGHT + B_FOLLOW_BOTTOM);
		
		button->SetEnabled(false);
		button->SetTarget(this);
		box->AddChild(button);
	}
	// enable remove button as needed
	button->SetEnabled(specViewList.CountItems() > 0);
}


/* Spec View */
SpecView::SpecView(BRect rect)
	:BView(rect, "SpecView", B_FOLLOW_NONE, B_WILL_DRAW)
{
	BRect			bounds;

	SetViewColor(200,200,200,255);
	SetLowColor(200,200,200,255);

	// add text entry boxes
	bounds = Bounds();
	bounds.right = bounds.left + 120;
	bounds.top += 2;
	fText1 = new BTextControl(bounds, "Text2", "", "", NULL);
	fText1->SetDivider(0.0);

	bounds = Bounds();
	bounds.left += 260;
	bounds.right = bounds.left + 120;
	bounds.top += 2;
	fText2 = new BTextControl(bounds, "Text1", "", "", NULL);
	fText2->SetDivider(0.0);


	BPopUpMenu *menu = new BPopUpMenu("PopUp");

	menu->SetRadioMode(true);
	menu->SetFont(be_plain_font);

	menu->AddItem(new BMenuItem("B_DIRECT_SPECIFIER", new BMessage(B_DIRECT_SPECIFIER)));
	menu->AddItem(new BMenuItem("B_INDEX_SPECIFIER", new BMessage(B_INDEX_SPECIFIER)));
	menu->AddItem(new BMenuItem("B_NAME_SPECIFIER", new BMessage(B_NAME_SPECIFIER)));
	menu->AddItem(new BMenuItem("B_REVERSE_INDEX_SPECIFIER", new BMessage(B_REVERSE_INDEX_SPECIFIER)));
	menu->AddItem(new BMenuItem("B_RANGE_SPECIFIER", new BMessage(B_RANGE_SPECIFIER)));
	menu->AddItem(new BMenuItem("B_REVERSE_RANGE_SPECIFIER", new BMessage(B_REVERSE_RANGE_SPECIFIER)));
	menu->AddItem(new BMenuItem("B_ID_SPECIFIER", new BMessage(B_ID_SPECIFIER)));

	// mark first item
	menu->ItemAt(0)->SetMarked(true);
	
	bounds = Bounds();
	bounds.left += 125;
	bounds.right = bounds.left + 130;
	bounds.bottom = bounds.top + 15;
	fMenu = new BMenuField(bounds, "MenuField", "", menu);
	fMenu->SetDivider(0.0);

}


SpecView::~SpecView()
{
}

void 
SpecView::AttachedToWindow()
{
	AddChild(fText1);
	AddChild(fText2);
	fText1->MakeFocus();

	AddChild(fMenu);
	BMenu *menu = fMenu->Menu();

	// target everything
	menu->SetTargetForItems(this);
}

void 
SpecView::MessageReceived(BMessage *msg)
{
	BView::MessageReceived(msg);
}

