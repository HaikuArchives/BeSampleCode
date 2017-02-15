/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Application.h>
#include <Roster.h>
#include <MenuItem.h>
#include <Mime.h>
#include <PopUpMenu.h>

#include "ModuleProxy.h"
#include "../simpleColorPicker/Protocol.h"
#include "ColorLabel.h"

ModuleProxy::ModuleProxy(ColorLabel *target, const char *type,
	const char *preferredApp)
	:	type(type),
		preferredApp(preferredApp),
		connectionOpen(false),
		target(target),
		lastInvoke(0)
{
}

void 
ModuleProxy::SetTarget(BLooper *looper)
{
	looper->AddHandler(this);
}


ModuleProxy::~ModuleProxy()
{
	if (connectionOpen)
		module.SendMessage(kCloseConnection);
}

void 
ModuleProxy::Invoke()
{
	if (connectionOpen) {
		module.SendMessage(kActivateWindow);
		// we already have a picker serving us, pull it up
		return;
	}

	bigtime_t now = system_time();
	if (now - lastInvoke < 1000000)
		// don't invoke again for a bit after a first invoke
		// to prevent launching two pickers
		// it would be a bit nicer to wait after kOpenConnection returns
		return;

	lastInvoke = now;

	uint32 buttons;
	BPoint point;
	target->GetMouse(&point, &buttons);
	
	BMessage launchMessage(kInitiateConnection);

	launchMessage.AddMessenger(kClientAddress, BMessenger(this));
		// this is the messenger we want the color picker to
		// interact with
	launchMessage.AddPoint(kInvokePoint, target->ConvertToScreen(point));
		// add the current invocation point so that the color picker
		// can position itself near the click
	launchMessage.AddString(kTargetName, target->Name());
		// add the current invocation point so that the color picker
		// can position itself near the click

	rgb_color color = target->ValueAsColor();
	launchMessage.AddData(kInitialValue, B_RGB_COLOR_TYPE, &color, sizeof(color));
		// add the current color value
	
	launchMessage.AddInt32(kRequestedValues, B_RGB_COLOR_TYPE);
		// ask for the type of values we need

	if (preferredApp.Length()) 
		// whe have a specific preferred appliacation for this instance
		// launch the picker - use the application signature for
		// this particular client
		be_roster->Launch(preferredApp.String(), &launchMessage);
	else
		be_roster->Launch(type.String(), &launchMessage);
			// launch the picker, just use the mime type
			// to choose the preferred application
}

void 
ModuleProxy::SetPreferredApp(const char *newPreferredApp)
{
	preferredApp = newPreferredApp;
}

void 
ModuleProxy::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_VALUE_CHANGED:
			{
				rgb_color *color;
				ssize_t size;
				if (message->FindData(kNewValue, B_RGB_COLOR_TYPE,
					(const void**) &color, &size) == B_OK)
					target->SetColor(*color);
			}
			break;

		case kOpenConnection:
			{
				connectionOpen = true;
				BMessenger messenger;
				if (message->FindMessenger(kServerAddress, &messenger) == B_OK)
					module = messenger;
			}
			break;

		case kServerQuitting:
			connectionOpen = false;
			break;

		default:
			_inherited::MessageReceived(message);
	}
}

void 
ModuleProxy::UpdateValue(rgb_color newValue)
{
	if (connectionOpen) {
		BMessage colorChanged(B_VALUE_CHANGED);
		colorChanged.AddData(kNewValue, B_RGB_COLOR_TYPE, &newValue,
			sizeof(newValue));
		module.SendMessage(&colorChanged);
	}
}

void 
ModuleProxy::RunPreferredPickerSelector(BPoint where)
{
	BPopUpMenu *menu = new BPopUpMenu("preferredApp");
	BMenuItem *item = new BMenuItem("Default", 0);
	menu->AddItem(item);
	menu->AddSeparatorItem();

	BMimeType mime(type.String());
	
	// build a list of all the supporting apps
	BMessage message;
	mime.GetSupportingApps(&message);
	for (int32 index =0; ; index++) {
		const char *signature;
		status_t reply = message.FindString("applications", index, &signature);
		
		if (reply != B_NO_ERROR || !signature || !signature[0])
			break;

		BMessage *tmp = new BMessage;
		tmp->AddString("signature", signature);
		
		entry_ref entry;
		if (be_roster->FindApp(signature, &entry) == B_OK)
			// add the application by its name
			item = new BMenuItem(entry.name, tmp);
		else
			// can't find the app, just use the signature
			item = new BMenuItem(signature, tmp);
		menu->AddItem(item);
		
		// mark the preferred app
		if (preferredApp.ICompare(signature) == 0)
			item->SetMarked(true);

	}

	if (!menu->FindMarked())
		// mark "Default"
		menu->ItemAt(0)->SetMarked(true);

	// make the selected signature preferred
	item = menu->Go(where);
	const char *signature;
	if (item) {
		if (!item->Message())
			// picked "Default"
			preferredApp = "";
		else if (item->Message()->FindString("signature", &signature) == B_OK)
			preferredApp = signature;
	}
	
	delete menu;
}

