/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
/*
 * Copyright (C) 1996 Be, Inc.  All Rights Reserved
 */
#include "generic.h"
#include <Alert.h>

enum control_codes {
	OKHIT = 0x1000,
	CANCELHIT
};

class GenericCWindow : public BWindow {
public:
	GenericCWindow(const char* interfaceName,
		const char *device, net_settings *ncw, BCallbackHandler* callback);

	void Finish(status_t status) {
		fCallbackHandler->Done(status);
		Lock();
		Quit();
	}
	void MessageReceived(BMessage *msg) {
		switch (msg->what) {
		case OKHIT:
			Finish(B_NO_ERROR);
			break;
		case CANCELHIT:
			Finish(B_ERROR);
			break;
		default:
			BWindow::MessageReceived(msg);
			break;
		}
	}
	friend filter_result kd_filter(BMessage *, BHandler **, BMessageFilter *);

private:
	const char*			fDevice;
	net_settings*		fNetSettings;
	BCallbackHandler*	fCallbackHandler;
};

// *********************************************************************************

static void
CenterWindowOnScreen(BWindow* w)
{
	BRect	screenFrame = (BScreen(B_MAIN_SCREEN_ID).Frame());
	BPoint 	pt;
	pt.x = screenFrame.Width()/2 - w->Bounds().Width()/2;
	pt.y = screenFrame.Height()/2 - w->Bounds().Height()/2;

	if (screenFrame.Contains(pt))
		w->MoveTo(pt);
}

static float
FontHeight(BView* target, bool full)
{
	font_height finfo;		
	target->GetFontHeight(&finfo);
	float h = finfo.ascent + finfo.descent;

	if (full)
		h += finfo.leading;
	
	return h;
}

// *********************************************************************************

GenericCWindow::GenericCWindow(const char* interfaceName,
	const char *device, net_settings *ncw, BCallbackHandler *callback)
	: BWindow(BRect(0,0,205, 75), "Net Add-On",
		B_MODAL_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_CLOSABLE) ,
			fDevice(device), fNetSettings(ncw), fCallbackHandler(callback)
{
	char buf[128];
	char tmp[5];
	int i;

	BRect r(Bounds());
	r.InsetBy(-1, -1);
	BBox* bg = new BBox(r, "", B_FOLLOW_ALL);
	bg->SetFont(be_plain_font);
	AddChild(bg);

	float fh = FontHeight(bg, true);
	
	r.left = 11; r.right = r.left + bg->StringWidth(interfaceName);
	r.top = 12; r.bottom = r.top + fh;
	BStringView* interfaceNameFld = new BStringView(r, "interface name", interfaceName);
	bg->AddChild(interfaceNameFld);
	
	r.top = interfaceNameFld->Frame().bottom + 11;
	r.right = Bounds().Width() - 10;
	r.bottom = r.top + fh;
	
	r.top = interfaceNameFld->Frame().bottom + 13;
	r.bottom = r.top + 1;
	r.right = Bounds().Width() - 10;
	r.left = r.right - 75;
	BButton* okayBtn = new BButton(r, "Cancel", "Cancel", new BMessage(OKHIT),
		B_FOLLOW_TOP | B_FOLLOW_RIGHT);
	okayBtn->MoveTo(r.left, Bounds().Height() - 10 - okayBtn->Bounds().Height());
	
	bg->AddChild(okayBtn);
	SetDefaultButton(okayBtn);
	
	BMessageFilter *filter = new BMessageFilter(B_KEY_DOWN, kd_filter);
	AddCommonFilter(filter);
	
	//	resize the window to accomodate a long interface name, if necessary
	if (interfaceNameFld->Bounds().Width() > Bounds().Width()) {
		ResizeTo(interfaceNameFld->Bounds().Width() + 10, 84);
	}
		
	CenterWindowOnScreen(this);
}

// *********************************************************************************

filter_result kd_filter(BMessage *msg, BHandler **target, BMessageFilter *f)
{
	GenericCWindow*	w = (GenericCWindow *) f->Looper();
	uchar ch = 0;

	if (msg->FindInt8("byte", (int8 *)&ch) == B_NO_ERROR) {
		if (ch == B_ESCAPE) {
			w->Finish(B_ERROR);
		}
	}

	return B_DISPATCH_MESSAGE;
}

status_t
GenericConfig::Config(
					  const char *device,
					  net_settings *ncw,
					  BCallbackHandler *callback,
					  bool autoconf
					  )
{
	GenericCWindow* win;
	
	if (!autoconf){ 
		win = new GenericCWindow(title, device, ncw, callback);
		win->Show();
	}
	
	return (B_NO_ERROR);
}


int
GenericDevice::Start(const char *device)
{
	char devicelink[NC_MAXVALLEN+1];
	ether_init_params_t params;

	if (!netconfig_find(device, "DEVICELINK", devicelink,
						sizeof(devicelink))) {
		return (B_ERROR);
	}

	return (Init(devicelink, (char *)&params));
}
