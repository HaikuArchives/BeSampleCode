/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef __SIMPLE_WINDOW__
#define __SIMPLE_WINDOW__

#include <Window.h>
#include <Box.h>

class SimpleView : public BBox {
public:
	SimpleView(BRect frame);

protected:
	virtual void MessageReceived(BMessage *);
	virtual void AttachedToWindow();

private:
	typedef BBox _inherited;
};

class SimpleWindow : public BWindow {
public:
	SimpleWindow(BPoint offset);

protected:
	virtual void MessageReceived(BMessage *);
	virtual bool QuitRequested();

private:

	typedef BWindow _inherited;
};

#endif
