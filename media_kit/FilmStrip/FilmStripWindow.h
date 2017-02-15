#ifndef _FILMSTRIPWINDOW_H_
#define _FILMSTRIPWINDOW_H_

#include <interface/Window.h>
#include <app/MessageRunner.h>
#include <interface/Slider.h>
#include <storage/Entry.h>
#include "FilmStrip.h"
#include "BitmapView.h"

class FilmStripWindow : public BWindow {
public:
	FilmStripWindow(const entry_ref* ref = NULL);
	virtual ~FilmStripWindow(void);
	
	virtual bool QuitRequested(void);
	virtual void MessageReceived(BMessage* msg);
	
private:
	void Prepare(const entry_ref& ref);

	FilmStrip* mStrip;
	BitmapView* mView;
	BMessageRunner* mSwitcher;
	BMessenger* mCWin;
	entry_ref mRef;
};

#endif // #ifndef _FILMSTRIPWINDOW_H_