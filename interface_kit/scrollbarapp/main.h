/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef __main_h
#define __main_h 1

#define APP_NAME "ScrollBarApp"

#define APP_SIG "application/x-vnd.Be-ScrollBarApp"

#include <Application.h>
#include <FilePanel.h>

#define T_OPEN 'open'
#define T_WRAP 'wrap'
#define T_EDGE 'edge'
#define T_SCALE 'scal'
#define T_TOFIT 'tfit'

#define T_FITMENU "Stretch to Fit"
#define T_WRAPMENU "Wrap Text"
#define T_EDGEMENU "Wrap to Edge"
#define T_SCALEMENU "Scale"

// This happens to be the size of a large icon
// The min size can be smaller, if the image itself is smaller.
const float min_length = 32.0;

// This is the amount of space on the screen that needs to be accounted for by title bars and borders
#define CRUFT_SIZE_X 12
#define CRUFT_SIZE_Y 32

class TApp : public BApplication {
public:
	TApp();
	virtual ~TApp();
	// virtual void AboutRequested(void);
	virtual void ArgvReceived(int32 argc, char **argv);
	virtual void ReadyToRun(void);
	virtual void Pulse(void);
	virtual void RefsReceived(BMessage *message);
	virtual void MessageReceived(BMessage *message);
private:
	BFilePanel *load;
};

#endif // __main_h
