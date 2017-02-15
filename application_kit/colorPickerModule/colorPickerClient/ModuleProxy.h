/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef __MODULE_PROXY__
#define __MODULE_PROXY__

#include <InterfaceDefs.h>
#include <Handler.h>
#include <Messenger.h>
#include <String.h>

class ColorLabel;

class ModuleProxy : public BHandler {
public:
	ModuleProxy(ColorLabel *target, const char *type,
		const char *preferredApp = 0);
	virtual ~ModuleProxy();

	void Invoke();
	
	void SetTarget(BLooper *);
	void SetPreferredApp(const char *);
	void UpdateValue(rgb_color);

	void RunPreferredPickerSelector(BPoint);
protected:
	virtual void MessageReceived(BMessage *);

private:

	BString type;
	BString preferredApp;
	
	bool connectionOpen;
	BMessenger module;
	ColorLabel *target;

	bigtime_t lastInvoke;

	typedef BHandler _inherited;
};

#endif
