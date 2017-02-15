/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Application.h>
#include <Window.h>
#include <View.h>

class BBitmap;

class BTSImagingView : public BView
{
public:
	BTSImagingView(BRect frame);
	
	virtual	void	Draw(BRect updateRect);
	virtual void	AttachedToWindow();
	
protected:
	BBitmap *fOffscreenImage;
	BView	*fOffscreenView;
	BBitmap	*fSourceImage;
	BBitmap	*fDestinationImage;

private:

};

class BTSImagingWindow : public BWindow
{
public:
	BTSImagingWindow();

	virtual	bool	QuitRequested();
	
protected:
	BView *fMainView;
		
private:

};

//=======================================================
// Class: BTSImagingApp
//
// This doesn't do much more than act as a harness
// which is to be used to exercise the imaging library
//=======================================================

class BTSImagingApp : public BApplication
{
public:
	BTSImagingApp();

protected:
	BWindow *fMainWindow;
};


