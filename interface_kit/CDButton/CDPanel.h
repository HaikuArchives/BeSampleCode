/*
	Copyright 1992-1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

// This defines the panel and the different buttons in the
// CDButton UI

#ifndef __CD_PANEL__
#define __CD_PANEL__

#include <Application.h>
#include <Box.h>
#include <View.h>
#include <Window.h>
#include <Invoker.h>

#include "TypedList.h"
#include "Observer.h"

class CDEngine;
class TrackState;
class PlayState;
class TimeState;
class VolumeState;

class CDPanelWindow : public BWindow {
public:
	CDPanelWindow(BPoint, CDEngine *);
};

struct PictDescriptor {
	// a little convenience class containing bitmap bits and the
	// location of the bitmap in the panel
	PictDescriptor(const uchar *bits, BPoint location, BPoint size)
		:	bits(bits),
			location(location),
			size(size)
		{}

	const uchar *bits;
	BPoint location;
	BPoint size;
};

class TrackableButton : public BView , public BInvoker {
	// Special button that works with our CD panel that is only shown while
	// the mouse is down
public:
	TrackableButton(BRect, const char *, PictDescriptor descriptor,
		BMessage *invokeMessage = 0, BMessage *startPressingMessage = 0,
		BMessage *pressedMessage = 0, BMessage *donePressingMessage = 0);
		// invokeMessage, if specified, gets called when the button is released
		// startPressingMessage, if specified, gets called when the button is
		//		first pressed
		// pressedMessage, if specified, gets called periodically while the
		// 		button is pressed
		// donePressingMessage, if specified, gets called when the button is
		//		not pressed any more; compared to invokeMessage, donePressingMessage
		//		gets called even when we track out of the button
		 
	virtual ~TrackableButton();
	virtual void Draw(BRect);
	

	virtual status_t StartPressing(BMessage *startPressingMessage = 0);
	virtual status_t Pressed(BMessage *pressedMessage = 0);
	virtual status_t DonePressing(BMessage *donePressingMessage = 0);
		// like invoke, called periodically when button pressed

	void Invert(bool);
protected:
	
	virtual BBitmap *CurrentPict() const;
	virtual BPoint CurrentPictLocation() const;

	bool inverted;
	BBitmap *pict;
	BPoint pictLocation;
	BMessage *pressedMessage;
	BMessage *startPressingMessage;
	BMessage *donePressingMessage;
};


class PlayButton : public TrackableButton, private Observer {
	// PlayButton is a little fancier in that it shows a different
	// bitmap depending on the play state the CD is in
public:
	PlayButton(BRect, const char *, PictDescriptor playPictDescriptor,
		PictDescriptor pausePictDescriptor, CDEngine *,
		BMessage *invokeMessage);
	~PlayButton();

	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage *);
	virtual BHandler *RecipientHandler() const
		{ return (BHandler *)this; }

	virtual void NoticeChange(Notifier *);
private:
	virtual BBitmap *CurrentPict() const;
	virtual BPoint CurrentPictLocation() const;

	BBitmap *pausePict;
	BPoint pausePictLocation;
	CDEngine *engine;
};

class TrackPopupButton : public TrackableButton {
	// This shows the popup menu with all the track numbers
public:
	TrackPopupButton(BRect, const char *, CDEngine *);
private:
	virtual status_t StartPressing(BMessage *);
	
	CDEngine *engine;
	BPopUpMenu *menu;
};

float
BlitDigit(BView *view, BPoint offset, uint32 digitIndex, BRect digitSize,
	const BBitmap *map, const rgb_color viewColor);
	// a convenience call that blits a digit from a bitmap into a BView at a certain offset

class InfoDisplay : public BView, public Observer {
	// used by TimeDisplay and TrackDisplay
	// it is an Observer so that it can subscribe to the CDEngine and get
	// notified whenever the value that it displays is changed
public:
	InfoDisplay(BRect, const char *);

	virtual void MessageReceived(BMessage *);
	virtual BHandler *RecipientHandler() const
		{ return (BHandler *)this; }

	virtual void NoticeChange(Notifier *);
protected:
	float DrawDigit(float offset, uint32 digitIndex, BRect digitSize,
		const BBitmap *map);

};

class TrackDisplay : public InfoDisplay  {
	// A view that gets notified from the CD engine and draws the
	// current track number
public:
	TrackDisplay(BPoint position, TrackState *);
	~TrackDisplay();

	virtual void AttachedToWindow();
	virtual void Draw(BRect);
	
private:
	BBitmap *segments;
	TrackState *trackState;
};

class TimeDisplay : public InfoDisplay  {
	// A view that gets notified from the CD engine and draws the
	// current location on the CD in minutes and seconds
public:
	TimeDisplay(BPoint , TimeState *);
	~TimeDisplay();

	virtual void AttachedToWindow();
	virtual void Draw(BRect);
	
private:
	BBitmap *segments;
	TimeState *timeState;
};

class CDPanel : public BBox {
	// this panel is displayed when the CDButton is pressed and while
	// the mouse is down; It handles tracking of all it's trackable
	// buttons
public:
	CDPanel(BRect, CDEngine *);
	virtual void MouseDown(BPoint where);
	virtual void MouseUp(BPoint where);
	virtual void Pulse();
	virtual void Draw(BRect);
	static BRect GetPosition(BPoint );
	void TrackMenu(BPopUpMenu *);
private:
	TrackableButton *AddButton(TrackableButton *);
	void Track();
		// track a button
	int32 InButtonIndex(BPoint) const;
		// convert point to button index
	
	TypedList<TrackableButton *> buttons;

	int32 lastHitIndex;
	bool wasDown;

	CDEngine *engine;
	BPopUpMenu *menu;
};

rgb_color ShiftColor(rgb_color color, float percent);
	// convenience call used to make a given color darker or lighter
#endif