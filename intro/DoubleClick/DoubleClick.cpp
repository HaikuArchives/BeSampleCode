// DoubleClick
//
// An application for demonstrating multiple-click behaviors in the BeOS.

#include "DoubleClick.h"
#include <stdio.h>

// view that reports its double-click status

DoubleClickView::DoubleClickView(BRect frame, const char *name, uint32 resizeMask, uint32 flags)
	: BView(frame, name, resizeMask, flags),
	mLastButton(0), mClickCount(0), mText("No clicks so far")
{
}

void 
DoubleClickView::Draw(BRect updateRect)
{
	// just display the click status text
	DrawString(mText.String(), BPoint(20, 30));
}

void 
DoubleClickView::MouseDown(BPoint where)
{
	BMessage* msg = Window()->CurrentMessage();
	int32 clicks = msg->FindInt32("clicks");
	int32 button = msg->FindInt32("buttons");

	// is this a continuing click sequence on the same button?  We can
	// detect this by ensuring that the button currently being reported
	// is the same as the previous one, and furthermore that the "clicks"
	// field is being incremented, i.e. that repeated mouse clicks are
	// taking place within the multiple-click timeout threshold.
	if ((button == mLastButton) && (clicks > 1))
	{
		mClickCount++;
	}
	else mClickCount = 1;		// no, it's the first click of a new button
	mLastButton = button;		// remember what the last button pressed was

	// buttons are a bitmask:  bit zero for the primary button, bit 1 for
	// the secondary button, etc.
	const char* buttonName = "Primary button";
	if (button == B_SECONDARY_MOUSE_BUTTON) buttonName = "Secondary button";
	else if (button == B_TERTIARY_MOUSE_BUTTON) buttonName = "Tertiary button";

	// names are more fun than simple click counts
	const char* clickName;
	switch (mClickCount)
	{
	case 1: clickName = "single click"; break;
	case 2: clickName = "double click"; break;
	case 3: clickName = "triple click"; break;
	default: clickName = "many clicks!"; break;
	}

	// display an appropriate message in the view
	char str[64];
	sprintf(str, "%s; %s", buttonName, clickName);
	mText = str;
	Invalidate();
}

// A window to contain the DoubleClickView
// #pragma mark -

DoubleClickWin::DoubleClickWin()
	: BWindow(BRect(50, 50, 250, 100), "DoubleClick", B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS)
{
	// the whole window is just one big DoubleClickView
	BView* v = new DoubleClickView(Bounds(), "DoubleClickView", B_FOLLOW_NONE, B_WILL_DRAW);
	AddChild(v);
}

void 
DoubleClickWin::Quit()
{
	// quit the app when the window is closed
	be_app->PostMessage(B_QUIT_REQUESTED);
	BWindow::Quit();
}

// the application to create the window
// #pragma mark -

DoubleClickApp::DoubleClickApp(const char *signature)
	: BApplication(signature)
{
}

void 
DoubleClickApp::ReadyToRun()
{
	// Create a window at startup
	DoubleClickWin* win = new DoubleClickWin();
	win->Show();
}

// A simple main() function to get it all going
// #pragma mark -

int main(int, char**)
{
	DoubleClickApp app("application/x-vnd.Be.DoubleClick");
	app.Run();

	return 0;
}

