#include <interface/Alert.h>
#include <support/String.h>
#include <app/Application.h>
#include <assert.h>
#include <interface/Bitmap.h>
#include <storage/Entry.h>
#include <interface/Screen.h>
#include <stdio.h>
#include "FilmStripWindow.h"
#include "BitmapView.h"
#include "ControlWindow.h"
#include "MessageVals.h"
#include "ErrorAlert.h"

const float kFudge = 10.0F; // account for window borders
const char* kCantOpenFileMsg = "Could not open movie file. "
				"It may not be a valid file, or it may require "
				"an unsupported codec.";

FilmStripWindow::FilmStripWindow(const entry_ref* ref)
	: BWindow(BRect(0,0,319,239),"FilmStrip",B_TITLED_WINDOW_LOOK,
			B_NORMAL_WINDOW_FEEL,B_ASYNCHRONOUS_CONTROLS
			| B_NOT_RESIZABLE),
		mStrip(ref == NULL ? NULL : new FilmStrip(*ref)),
		mView(NULL), mSwitcher(NULL), mCWin(NULL)
{
	status_t err;
	
	mView = new BitmapView(Bounds());
	AddChild(mView);

	MoveTo(50,50);
	
	if (ref != NULL) {
		mRef = *ref;
	}
	
	if (mStrip != NULL) {
		if ((err = mStrip->InitCheck()) != B_OK) {
			(new ErrorAlert(kCantOpenFileMsg,err))->Go();
		}
		else {
			Prepare(mRef);
		}
	}
}

FilmStripWindow::~FilmStripWindow(void)
{
	delete mStrip;
	delete mSwitcher;
	delete mCWin;
}

bool FilmStripWindow::QuitRequested(void)
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	
	return true;
}

void FilmStripWindow::MessageReceived(BMessage* msg)
{
	status_t err;
	entry_ref ref;
	int64 temp;

	switch (msg->what) {
	case B_SIMPLE_DATA:
		if (msg->FindRef("refs",&ref) == B_OK) {
			if (ref == mRef) {
				// bail early if it's the same file
				break;
			}
			if (FilmStrip::SniffRef(ref) != B_OK) {
				// bail early if not a movie file
				break;
			}
			
			// stop sending "show next frame" messages
			delete mSwitcher;
			mSwitcher = NULL;
			
			delete mStrip;
			mStrip = NULL;
			mStrip = new FilmStrip(ref,kFrame,2000000);
			if (mStrip != NULL) {
				if ((err = mStrip->InitCheck()) != B_OK) {
					// make sure we don't dereference invalid
					// pointers later on
					mView->SetImage(NULL);
					delete mStrip;
					mStrip = NULL;

					// set title back to default
					SetTitle("FilmStrip");

					// break sad news to the user
					(new ErrorAlert(kCantOpenFileMsg,
						err))->Go();
				}
				else {
					mRef = ref;
					Prepare(mRef);
				}
			}
		}
		else {
			// ensure expected window behavior if we don't handle
			// the message
			BWindow::MessageReceived(msg);
		}
		break;
	case kShowNextFrame:
		if (mStrip != NULL) {
			// tell the view there's a new frame to draw
			mView->SetImage(mStrip->NextFrame());
		}
		break;
	case kDrawIntvlChg:
		msg->FindInt64("GrabInterval",&temp);
		mSwitcher->SetInterval(temp);
		break;
	case kModeChg:
		mStrip->SetMode(msg->FindInt32("Mode"));
		printf("mode is now %ld\n",mStrip->Mode());
		break;
	case kGrabIntvlChg: {
		// value represents percentage of track duration
		mStrip->SetFrameInterval(
			(bigtime_t)(msg->FindInt32("FrameInterval")
				/ 100.0 * mStrip->Duration()));
		printf("frame interval is now %Ld\n",
			mStrip->FrameInterval());
		break;
	}
	default:
		// ensure expected window behavior if we don't handle
		// the message
		BWindow::MessageReceived(msg);
		break;
	}
}

void FilmStripWindow::Prepare(const entry_ref& ref)
{
	assert(mStrip != NULL);
		
	// make the window fit the movie
	ResizeTo(mStrip->Format().Width()-1,
		mStrip->Format().Height()-1);

	// set window's title to file's name
	BString title(ref.name);
	SetTitle(title.String());

	// tell the view what to draw
	mView->SetImage(mStrip->NextFrame());

	// start sending periodic "show next frame" msgs
	mSwitcher = new BMessageRunner(BMessenger(this),
		new BMessage(kShowNextFrame),1000000);

	// set up the control window
	if (mCWin == NULL) {
		BRect frame(Frame());
		BPoint offset(frame.right + kFudge,frame.top);
		BScreen screen(this);
		
		if (!screen.Frame().Contains(offset)) {
			offset.x = offset.y = 50;
		}
		ControlWindow* cWin = new ControlWindow(offset,this);
		mCWin = new BMessenger(cWin);
		cWin->Show();
	}

	// send new FilmStrip message
	mCWin->SendMessage(kNewFilmStrip);
}