/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
// This defines the replicant button

#ifndef __CD_BUTTON__
#define __CD_BUTTON__

#include <Application.h>
#include <View.h>
#include <Window.h>
#include <View.h>

#include "Observer.h"
#include "CDEngine.h"

class _EXPORT CDButton;
	// the dragger part has to be exported

class CDButton : public BView, private Observer {
public:
	CDButton(BRect frame, const char *name, int devicefd,
		uint32 resizeMask = B_FOLLOW_ALL, 
		uint32 flags = B_WILL_DRAW | B_NAVIGABLE | B_PULSE_NEEDED);
		
	CDButton(BMessage *);
		// BMessage * based constructor needed to support archiving
	virtual ~CDButton();

	// archiving overrides
	static CDButton *Instantiate(BMessage *data);
	virtual	status_t Archive(BMessage *data, bool deep = true) const;


	// misc BView overrides
	virtual void AttachedToWindow();	
	virtual void MouseDown(BPoint);
	
	virtual void Pulse();
	virtual void Draw(BRect );

	virtual void MessageReceived(BMessage *);

	// observing overrides
	virtual BHandler *RecipientHandler() const
		{ return (BHandler *)this; }

	virtual void NoticeChange(Notifier *);

	static bool RunningInDeskbar();
	
	static void DeskbarAddRemove(bool add);
	
private:
	CDEngine engine;
	BBitmap *segments;
};


class CDButtonApplication : public BApplication {
public:
	CDButtonApplication();
};


#endif