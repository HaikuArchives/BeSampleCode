/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef __win_h
#define __win_h 1

#include <Window.h>
#include <Path.h>
#include <MenuBar.h>
#include <Bitmap.h>

class TImageView;

class TWin : public BWindow {
public:
	TWin(BRect frame, const char *filename);
	~TWin();
	// virtual void FrameMoved(BPoint scrPt);
	virtual void FrameResized(float w, float h);
	// virtual void MenusBeginning(void);
	// virtual void MenusEnding(void);
	// virtual void Minimize(bool min);
	virtual void ScreenChanged(BRect frame, color_space mode);
	// virtual void WindowActivated(bool active);
	// virtual void WorkspaceActivated(int32 workspc, bool active);
	// virtual void WorkspaceChanged(int32 oldworkspc, int32 newworkspc);
	// virtual void Zoom(BPoint lefttop, float wide, float high);
	virtual void MessageReceived(BMessage *message);
	//// Additional stuffs
	status_t AddScrollingImage(BRect frame);
	status_t AddScrollingText(BRect frame);
	void RedoSizes(void);
private:
	enum viewtype_type {VIEWTYPE_ERROR = -1, VIEWTYPE_NONE, VIEWTYPE_TEXT, VIEWTYPE_IMAGE, VIEWTYPE_UNKNOWN};
	viewtype_type viewtype;
	BPath path;
	BMenuBar *menu;
	BTextView *text;
	bool WrapAtEdge;
	float filewidth;
	TImageView *imageview;
	BBitmap *image;
};

#endif // __win_h
