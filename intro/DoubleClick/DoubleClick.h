// DoubleClick.h

#ifndef DoubleClick_H
#define DoubleClick_H 1

#include <be/interface/View.h>
#include <be/interface/Window.h>
#include <be/app/Application.h>
#include <be/support/String.h>

class DoubleClickView : public BView
{
public:
	DoubleClickView(BRect frame, const char* name, uint32 resizeMask, uint32 flags);

	void Draw(BRect updateRect);
	void MouseDown(BPoint where);

private:
	int32 mLastButton;
	int32 mClickCount;
	BString mText;
};

class DoubleClickWin : public BWindow
{
public:
	DoubleClickWin();
	void Quit();
};

class DoubleClickApp : public BApplication
{
public:
	DoubleClickApp(const char* signature);
	void ReadyToRun();
};

#endif
