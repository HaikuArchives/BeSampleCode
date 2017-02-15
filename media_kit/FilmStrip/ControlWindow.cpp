#include <interface/Box.h>
#include <app/Message.h>
#include <support/String.h>
#include "ControlWindow.h"
#include "MessageVals.h"

ControlWindow::ControlWindow(const BPoint offset,const BWindow* const win)
	: BWindow(BRect(0,0,229,149).OffsetBySelf(offset),"Controls",
			B_FLOATING_WINDOW_LOOK,B_FLOATING_APP_WINDOW_FEEL,
			B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_NOT_RESIZABLE
			| B_ASYNCHRONOUS_CONTROLS),
		mFSWin(new BMessenger(win))
{
	BRect r(Bounds());

	// set up root view for all controls
	BView* background = new BView(r,"background",B_FOLLOW_ALL,
		B_WILL_DRAW);
	background->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(background);
	
	// set up Draw Interval slider
	r.InsetBy(5,5);
	r.bottom = r.top + 50;
	mGrabIntvlSlider = new BSlider(r,"DrawIntvlSlider",
		"Grab Next Frame every (seconds)",new BMessage(kDrawIntvlChg),
		1,120);
	mGrabIntvlSlider->SetModificationMessage(
		new BMessage(*(mGrabIntvlSlider->Message())));
	// range of 1-120 represents 0.25 to 30 secs.
	mGrabIntvlSlider->SetLimitLabels("0.25","30");
	// set to default value
	mGrabIntvlSlider->SetValue(4/* i.e., 1 sec. */);
	background->AddChild(mGrabIntvlSlider);

	// set up Drop Frames checkbox
	r.OffsetBy(0,55);
	mDropFramesChkBox = new BCheckBox(r,"DropFramesChkBox",
		"Drop Frames",new BMessage(kModeChg));
	mDropFramesChkBox->ResizeToPreferred();
	// set to default value
	mDropFramesChkBox->SetValue(B_CONTROL_OFF);
	
	// set up group box
	r.bottom = Bounds().bottom - 5;
	BBox* box = new BBox(r,"group box");
	box->SetLabel(mDropFramesChkBox);
	background->AddChild(box);
	
	// set up Grab Interval slider
	r = box->Bounds();
	r.InsetBy(5,5);
	r.top += 20;
	mFrameIntvlSlider = new BSlider(r,"GrabIntvlSlider",
		"Media Time Between Frames (% of duration)",
		new BMessage(kGrabIntvlChg),0,100);
	mFrameIntvlSlider->SetModificationMessage(
		new BMessage(*(mFrameIntvlSlider->Message())));
	mFrameIntvlSlider->SetLimitLabels("0","100");
	// set to default value
	mFrameIntvlSlider->SetValue(10/* 10% of duration */);
	box->AddChild(mFrameIntvlSlider);
}

ControlWindow::~ControlWindow(void)
{
	delete mFSWin;
}

void ControlWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case kNewFilmStrip:
		// make all the controls send messages to
		// the FilmStrip so it gets set to current
		// control values
		PostMessage(kDrawIntvlChg);
		PostMessage(kGrabIntvlChg);
		PostMessage(kModeChg);
		break;
	case kDrawIntvlChg:
		// range of 1-120 represents 0.25 to 30 secs.
		msg->AddInt64("GrabInterval",
			mGrabIntvlSlider->Value()*1000000/4);
		mFSWin->SendMessage(msg);
		break;
	case kGrabIntvlChg:
		msg->AddInt32("FrameInterval",
			mFrameIntvlSlider->Value());
		mFSWin->SendMessage(msg);
		break;
	case kModeChg:
		msg->AddInt32("Mode",mDropFramesChkBox->Value());
		mFSWin->SendMessage(msg);
		mFrameIntvlSlider->SetEnabled(
			mDropFramesChkBox->Value() == B_CONTROL_ON);
		break;
	default:
		BWindow::MessageReceived(msg);
		break;
	}
}