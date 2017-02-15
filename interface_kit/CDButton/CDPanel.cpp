/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Bitmap.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Screen.h>
#include <String.h>

#include <vector>

#include "CDPanel.h"
#include "CDEngine.h"
#include "iconfile.h"

// The CDPanel uses a special mode of tracking the controling buttons.
// The panel goes away when you let go of the mouse button and only
// lets you track the buttons while the mouse is down. A button is
// invoked by releasing the mouse while over a button. Skip forward
// and back buttons acutally cause skipping while you are pressing them
// and stop skipping when you track out of the button with the mouse.
//
// Note that the ATAPI driver shipped with systems up to R3 does not
// implement skipping properly. This will be fixed in R4.


const float kDarkness = 1.06;
	// this constant controls the overall panel shade, including the buttons

BRect kPanelRect(0, 0, 135, 75);

// define all the different pictures for the buttons
const PictDescriptor kPlayPict(play16x10_raw, BPoint(25, 9), BPoint(16, 10));
const PictDescriptor kPausePict(pause16x10_raw, BPoint(25, 9), BPoint(16, 10));
const PictDescriptor kStopPict(stop8x8_raw, BPoint(11, 9), BPoint(8, 8));
const PictDescriptor kEjectPict(eject16x11_raw, BPoint(8, 8), BPoint(16, 11));
const PictDescriptor kSkipLeftPict(skipleft24x8_raw, BPoint(8, 5), BPoint(24, 8));
const PictDescriptor kSkipRightPict(skipright24x8_raw, BPoint(10, 5), BPoint(24, 8));
const PictDescriptor kSkipTrackLeftPict(skiptrackleft24x8_raw, BPoint(8, 5), BPoint(24, 8));
const PictDescriptor kSkipTrackRightPict(skiptrackright24x8_raw, BPoint(10, 5), BPoint(24, 8));
const PictDescriptor kEmptyPict(0, BPoint(0, 0), BPoint(0, 0));

const BPoint kTrackDisplayLocation(89, 38);
const BPoint kTimeDisplayLocation(89, 58);

const rgb_color kGray = { 220, 220, 220, 0 };

BRect 
CDPanel::GetPosition(BPoint origin)
{
	// get a window position, respecting the screen frame
	const int32 kMargin = 3;

	BRect screenFrame(BScreen(B_MAIN_SCREEN_ID).Frame());
	
	BRect result(kPanelRect);
	result.OffsetBy(origin);
	
	if (screenFrame.right < result.right + kMargin)
		result.OffsetBy(- kMargin - result.right + screenFrame.right, 0);
	
	if (screenFrame.bottom < result.bottom + kMargin)
		result.OffsetBy(0, - kMargin - result.bottom + screenFrame.bottom);

	if (screenFrame.left > result.left - kMargin)
		result.OffsetBy(kMargin + screenFrame.left - result.left, 0);
	
	if (screenFrame.top > result.top - kMargin)
		result.OffsetBy(0, kMargin + screenFrame.top - result.top);

	return result;
}

CDPanelWindow::CDPanelWindow(BPoint origin, CDEngine *engine)
	:	BWindow(CDPanel::GetPosition(origin), "CDPanel", B_BORDERED_WINDOW, 0)
{
	AddChild(new CDPanel(Bounds(), engine));
	SetPulseRate(100000);
		// need some quick tracking action here
}

CDPanel::CDPanel(BRect rect, CDEngine *engine)
	:	BBox(BRect(	rect.left,
					rect.top,
					rect.right + 1,
					rect.bottom + 1
					), "", B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED),
		engine(engine)
{
	// set up the whole CD panel with all the buttons
	BRect brect;

	// Stop button
	brect = BRect(0, 0, 30, 25);
	brect.OffsetTo(5, 5);
	AddButton(new TrackableButton(brect, "stop", kStopPict,
		CDEngineFunctorFactory::NewFunctorMessage(&CDEngine::Stop, engine)));
		// for all these buttons we pass a function object message as the
		// invoke message - as a result, when the time comes the respective
		// routine gets called

	// Play/pause button
	brect = BRect(0, 0, 60, 25);
	brect.OffsetTo(40, 5);
	AddButton(new PlayButton(brect, "play", kPlayPict, kPausePict, engine,
		CDEngineFunctorFactory::NewFunctorMessage(&CDEngine::PlayOrPause, engine)));
	
	// Eject
	brect = BRect(0, 0, 25, 25);
	brect.OffsetTo(105, 5);
	AddButton(new TrackableButton(brect, "eject", kEjectPict,
		CDEngineFunctorFactory::NewFunctorMessage(&CDEngine::Eject, engine)));

	// skip a bit left
	brect = BRect(0, 0, 35, 15);
	brect.OffsetTo(5, 55);
	AddButton(new TrackableButton(brect, "skipLeft", kSkipLeftPict, 0,
		CDEngineFunctorFactory::NewFunctorMessage(&CDEngine::StartSkippingBackward, engine), 0,
		CDEngineFunctorFactory::NewFunctorMessage(&CDEngine::StopSkipping, engine)));

	// skip a bit right
	brect.OffsetTo(45, 55);
	AddButton(new TrackableButton(brect, "skipRight", kSkipRightPict, 0,
		CDEngineFunctorFactory::NewFunctorMessage(&CDEngine::StartSkippingForward, engine), 0,
		CDEngineFunctorFactory::NewFunctorMessage(&CDEngine::StopSkipping, engine)));

	// skip a whole track left
	brect.OffsetTo(5, 35);
	AddButton(new TrackableButton(brect, "skipTrackLeft", kSkipTrackLeftPict,
		CDEngineFunctorFactory::NewFunctorMessage(&CDEngine::SkipOneBackward, engine)));


	// skip a whole track right
	brect.OffsetTo(45, 35);
	AddButton(new TrackableButton(brect, "skipTrackRight", kSkipTrackRightPict,
		CDEngineFunctorFactory::NewFunctorMessage(&CDEngine::SkipOneForward, engine)));

	// time and track display items
	AddChild(new TrackDisplay(kTrackDisplayLocation, engine->TrackStateWatcher()));
	AddChild(new TimeDisplay(kTimeDisplayLocation, engine->TimeStateWatcher()));

	// track popup menu
	brect = BRect(0, 0, 15, 15);
	brect.OffsetTo(115, 35);
	AddButton(new TrackPopupButton(brect, "", engine));

	lastHitIndex = -1;
	wasDown = true;

	// set up the panel color
	SetViewColor(ShiftColor(kGray, kDarkness));
	SetLowColor(ShiftColor(kGray, kDarkness));
}

TrackableButton *
CDPanel::AddButton(TrackableButton *button)
{
	buttons.AddItem(button);
	AddChild(button);
	button->SetTarget(engine);
	return button;
}

void
CDPanel::MouseUp(BPoint )
{
	// kill the window
	Window()->Lock();
	Window()->Quit();
}

void
CDPanel::MouseDown(BPoint )
{
	Track();
}

void 
CDPanel::Pulse()
{
	if (wasDown)
		// keep tracking while mouse is down
		Track();
}

int32
CDPanel::InButtonIndex(BPoint where) const
{
	// convert point to button index
	int32 count = buttons.CountItems();
	for (int32 index = 0; index < count; index++) 
		if (buttons.ItemAt(index)->Frame().Contains(where))
			return index;
	return -1;
}

void
CDPanel::Track()
{
	// track a button
	uint32 mouseButtons;
	BPoint where;
	GetMouse(&where, &mouseButtons, true);

	int32 inButton = InButtonIndex(where);

	// first update the graphical feedback for the tracking state
	if (inButton != lastHitIndex) {
		if (lastHitIndex >= 0)
			// un-invert whatever button was last inverted and
			// is no longer pressed
			buttons.ItemAt(lastHitIndex)->Invert(false);
		if (inButton >= 0)
			// invert the new button that was just pressed
			buttons.ItemAt(inButton)->Invert(true);
	}
	
	if (!mouseButtons) {
		// we are done pressing, send out the done pressing messages
		wasDown = false;
		if (lastHitIndex >= 0) 
			buttons.ItemAt(lastHitIndex)->DonePressing();
		if (inButton >= 0) {
			buttons.ItemAt(inButton)->DonePressing();
			buttons.ItemAt(inButton)->Invoke();
		}
		// and quit the window
		lastHitIndex = -1;
		Window()->Lock();
		Window()->Quit();
		return;
	}
	
	wasDown = true;
	if (inButton != lastHitIndex) {
		// the button changed
		if (lastHitIndex >= 0)
			// send done pressing from the old button 
			buttons.ItemAt(lastHitIndex)->DonePressing();
		if (inButton >= 0)
			// send start pressing from the new button
			buttons.ItemAt(inButton)->StartPressing();
		lastHitIndex = inButton;
	} else if (inButton >= 0)
		// we are still in the same button, send the periodic pressing message
		buttons.ItemAt(inButton)->Pressed();
}


const int kTrackDigitSpacing = 3;
const BRect kTrackDigitRect(0, 0, 7, 14);
const BRect kTrackDisplayRect(0, 0, 18, 14);

const int kTimeDigitSpacing = 5;
const BRect kTimeDigitRect(0, 0, 4, 8);
const BRect kTimeDisplayRect(0, 0, 6*4 + kTimeDigitSpacing - 1, 8);

void 
CDPanel::Draw(BRect rect)
{
	BBox::Draw(rect);

	// draw the track and time display outline

	rgb_color white = {255, 255, 255, 0};
	rgb_color darkerGray = ShiftColor(ViewColor(), 1.4);

	// draw the frame around the time display and track display views
	BRect trackRect(kTrackDisplayRect);
	trackRect.OffsetTo(kTrackDisplayLocation);
	trackRect.InsetBy(-4, -3);

	BRect timeRect(kTimeDisplayRect);
	timeRect.OffsetTo(kTimeDisplayLocation);
	timeRect.InsetBy(-4, -3);

	BeginLineArray(6);
	AddLine(trackRect.LeftTop(), trackRect.RightTop(), darkerGray);
	AddLine(trackRect.RightTop(), BPoint(trackRect.right, timeRect.top), white);
	AddLine(BPoint(trackRect.right, timeRect.top), timeRect.RightTop(), darkerGray);
	AddLine(timeRect.RightTop(), timeRect.RightBottom(), white);
	AddLine(timeRect.RightBottom(), timeRect.LeftBottom(), white);
	AddLine(timeRect.LeftBottom(), trackRect.LeftTop(), darkerGray);
	EndLineArray();
}

TrackableButton::TrackableButton(BRect frame, const char *name,
	PictDescriptor descriptor,
	BMessage *invokeMessage, BMessage *startPressingMessage,
	BMessage *pressedMessage, BMessage *donePressingMessage)
	:	BView(frame, name, 0, B_WILL_DRAW),
		BInvoker(invokeMessage, NULL),
		inverted(false),
		pressedMessage(pressedMessage),
		startPressingMessage(startPressingMessage),
		donePressingMessage(donePressingMessage)
{
	// set up the bitmap each button uses during draw
	pict = 0;
	pictLocation = BPoint(0, 0);
	if (descriptor.bits) {
		BRect rect(0, 0, descriptor.size.x - 1, descriptor.size.y - 1);

		pict = new BBitmap(rect, B_COLOR_8_BIT);
		pict->SetBits(descriptor.bits, descriptor.size.x * descriptor.size.y,
			0, B_COLOR_8_BIT);
		pictLocation = descriptor.location;
	}
}

TrackableButton::~TrackableButton()
{
	delete pict;
}

BBitmap *
TrackableButton::CurrentPict() const
{
	return pict;
}

BPoint 
TrackableButton::CurrentPictLocation() const
{
	return pictLocation;
}


const float kButtonShade = 0.9;

void
TrackableButton::Draw(BRect rect)
{
	// some code to draw a sculpted button in pressed or released
	// state
	BView::Draw(rect);
	BRect bounds(Bounds());	
	bool enabled = true;
	rgb_color c;

	c.blue = c.green = c.red = enabled ? (uint8)(kButtonShade * 50) : (uint8)(kButtonShade * 188);
	c = ShiftColor(c, kDarkness);

	// outline the button boundary
	BeginLineArray(4);
	BRect b = bounds;
	AddLine(BPoint(b.left, b.top+1), BPoint(b.left, b.bottom-1), c);
	AddLine(BPoint(b.left+1, b.bottom), BPoint(b.right-1, b.bottom), c);
	AddLine(BPoint(b.right, b.bottom-1), BPoint(b.right, b.top+1), c);
	AddLine(BPoint(b.right-1, b.top), BPoint(b.left+1, b.top), c);
	EndLineArray();
	
	// draw the button sides
	bounds = Bounds();
	BeginLineArray(8);

	c.red = c.blue = c.green = (uint8)(kButtonShade * 219);
	c = ShiftColor(c, kDarkness);

	AddLine(bounds.LeftTop(), bounds.LeftTop(), c);
	AddLine(bounds.RightTop(), bounds.RightTop(), c);
	AddLine(bounds.LeftBottom(), bounds.LeftBottom(), c);
	AddLine(bounds.RightBottom(), bounds.RightBottom(), c);

	// next rectangle in with light gray left, top and dark right, bottom
	bounds.InsetBy(1, 1);
	if (inverted) {
		// draw the sides for the inverted state
		c.red = c.green = c.blue = (uint8)(kButtonShade * 80);
		c = ShiftColor(c, kDarkness);

		AddLine(bounds.LeftTop(), bounds.RightTop(), c);
		AddLine(bounds.LeftTop(), bounds.LeftBottom(), c);

		c.red = c.green = c.blue = (uint8)(kButtonShade * 200);
		c = ShiftColor(c, kDarkness);

		AddLine(bounds.RightTop(), bounds.RightBottom(), c);
		AddLine(bounds.LeftBottom(), bounds.RightBottom(), c);
	} else {
		// draw the sides for the normal state
		if (enabled) 
			c.red = c.green = c.blue = (uint8)(kButtonShade * 148);
		else
			c.red = c.green = c.blue = (uint8)(kButtonShade * 200);
		c = ShiftColor(c, kDarkness);

		AddLine(bounds.RightTop(), bounds.RightBottom(), c);
		AddLine(bounds.LeftBottom(), bounds.RightBottom(), c);
		c.red = c.green = c.blue = (uint8)(kButtonShade * 232);
		c = ShiftColor(c, kDarkness);
		AddLine(bounds.LeftTop(), bounds.RightTop(), c);
		AddLine(bounds.LeftTop(), bounds.LeftBottom(), c);
	}
	EndLineArray();

	bounds.InsetBy(1, 1);
	// next rect in with slightly lighter right, bottom and white elsewhere
	if (inverted) {
		c.red = c.green = c.blue = (uint8)(kButtonShade * 160);
		SetHighColor(ShiftColor(c, kDarkness));
		bounds.right--;
		bounds.bottom--;
		StrokeLine(bounds.LeftBottom(), bounds.LeftTop());
		StrokeLine(bounds.RightTop(), bounds.LeftTop());
	} else {

		if (enabled)
			c.red = c.green = c.blue = (uint8)(kButtonShade * 188);
		else
			c.red = c.green = c.blue = (uint8)(kButtonShade * 210);
		SetHighColor(ShiftColor(c, kDarkness));

		bounds.left++;
		bounds.top++;
		StrokeLine(bounds.RightTop(), bounds.RightBottom());
		StrokeLine(bounds.LeftBottom(), bounds.RightBottom());
	}

	bounds.InsetBy(1, 1);

	// main interior of button
	if (inverted)
		c.red = c.green = c.blue = (uint8)(kButtonShade * 220);
	else
		c.red = c.green = c.blue = (uint8)(kButtonShade * 232);
	SetHighColor(ShiftColor(c, kDarkness));
	FillRect(bounds);
	
	if (CurrentPict()) {
		// draw the button pict; we are relying on parts of the pict bitmap
		// being transparent here
		SetDrawingMode(B_OP_OVER);
		SetLowColor(B_TRANSPARENT_32_BIT);
		DrawBitmap(CurrentPict(), CurrentPictLocation());
	}
}

status_t
TrackableButton::StartPressing(BMessage *message)
{
	// send the start pressing messages
	if (message)
		return Invoke(message);
	if (startPressingMessage)
		return Invoke(startPressingMessage);
	return B_OK;
}

status_t
TrackableButton::Pressed(BMessage *message)
{
	// send the start pressed messages
	if (message)
		return Invoke(message);
	if (pressedMessage)
		return Invoke(pressedMessage);
	return B_OK;
}

status_t
TrackableButton::DonePressing(BMessage *message)
{
	// send the done pressing messages
	if (message)
		return Invoke(message);
	if (donePressingMessage)
		return Invoke(donePressingMessage);
	return B_OK;
}

void
TrackableButton::Invert(bool invert)
{
	// invert the button
	if (inverted == invert)
		return;

	inverted = invert;
	// force a redraw
	Invalidate();
}


TrackPopupButton::TrackPopupButton(BRect rect, const char *name, CDEngine *engine)
	:	TrackableButton(rect, name, kEmptyPict),
		engine(engine),
		menu(0)
{
}

status_t 
TrackPopupButton::StartPressing(BMessage *)
{
	// handle start pressing by showing the track popup menu
	if (menu)
		return B_OK;

	menu = new BPopUpMenu("tracks");
	int32 current = engine->TrackStateWatcher()->GetTrack();
	int32 count = engine->TrackStateWatcher()->GetNumTracks();

	BString title;
	vector<BString> trackNames;
	bool gotNames = engine->ContentWatcher()->GetContent(&title, &trackNames);
	
	if (gotNames) {
		BMenuItem *item = new BMenuItem(title.String(), NULL);
		menu->AddItem(item);
		item->SetEnabled(false);
		menu->AddSeparatorItem();
	}
	
	// add an item for every track
	for (int32 index = 0; index < count; index++) {
		char buffer[256];
		sprintf(buffer, "%ld %s", index + 1, gotNames ? trackNames[index].String() : "");
			// ToDo:
			// clean the above up
		BMessage *message = new BMessage('slTk');
		message->AddInt32("track", index + 1);
		BMenuItem *item = new BMenuItem(buffer, message);
		if (index + 1 == current)
			item->SetMarked(true);
		
		item->SetTarget(engine);
		menu->AddItem(item);
	}

	BPoint where(0, 0);
	ConvertToScreen(&where);
	
	// show the menu
	menu->Go(where, true, false, true);
	return B_OK;
}

PlayButton::PlayButton(BRect frame, const char *name, PictDescriptor playPictDescriptor,
	PictDescriptor pausePictDescriptor, CDEngine *engine, BMessage *invokeMessage)
	:	TrackableButton(frame, name, playPictDescriptor, invokeMessage),
		engine(engine)
{
	// play button shows different bitmaps depending on the state of the CD
	pausePict = 0;
	pausePictLocation = BPoint(0, 0);
	if (pausePictDescriptor.bits) {
		BRect rect(0, 0, pausePictDescriptor.size.x - 1, pausePictDescriptor.size.y - 1);

		pausePict = new BBitmap(rect, B_COLOR_8_BIT);
		pausePict->SetBits(pausePictDescriptor.bits,
			pausePictDescriptor.size.x * pausePictDescriptor.size.y,
			0, B_COLOR_8_BIT);
		pausePictLocation = pausePictDescriptor.location;
	}

}

void
PlayButton::AttachedToWindow()
{
	// make the play button observe the play state
	StartObserving(engine->PlayStateWatcher());
}

PlayButton::~PlayButton()
{
	StopObserving();
	delete pausePict;
}

void
PlayButton::MessageReceived(BMessage *message)
{
	// hook up observing
	if (!Observer::HandleObservingMessages(message))
		TrackableButton::MessageReceived(message);
}

void 
PlayButton::NoticeChange(Notifier *)
{
	// respond to any notification by forcing a redraw
	Invalidate();
}

BBitmap *
PlayButton::CurrentPict() const
{
	// the pict used for drawing the play button depends on
	// the current CD play state
	if (engine->PlayStateWatcher()->GetState() == kPlaying)
		return pausePict;

	return pict;
}

BPoint 
PlayButton::CurrentPictLocation() const
{
	// get the picture location, corresponding to the picture
	// for the different play states
	if (engine->PlayStateWatcher()->GetState() == kPlaying)
		return pausePictLocation;

	return pictLocation;
}


InfoDisplay::InfoDisplay(BRect frame, const char *name)
	:	BView(frame, name, 0, B_WILL_DRAW)
{
}

void 
InfoDisplay::MessageReceived(BMessage *message)
{
	// hook up observing
	if (!Observer::HandleObservingMessages(message))
		BView::MessageReceived(message);
}

void 
InfoDisplay::NoticeChange(Notifier *)
{
	// respond to any notification by forcing a redraw
	Invalidate();
}

float
BlitDigit(BView *view, BPoint offset, uint32 digitIndex, BRect digitSize,
	const BBitmap *map, const rgb_color viewColor)
{
	// draw the 7-segment digit by blitting from a bitmap

	BRect src(digitSize);
	
		// the '-' is first in the digit array, used if no CD
	src.OffsetBy(digitIndex * (digitSize.Width() + 1), 0);
	BRect dst = view->Bounds();
	dst.OffsetTo(offset);
	dst.right = dst.left + digitSize.Width();
	dst.bottom = dst.top + digitSize.Height();
	view->SetHighColor(viewColor);
	view->SetViewColor(viewColor);
	view->FillRect(dst);
	view->SetHighColor(0, 0, 0);

	// take advantages of parts of the digit being transparent
	view->SetDrawingMode(B_OP_OVER);
	view->SetLowColor(B_TRANSPARENT_32_BIT);
	view->DrawBitmap(map, src, dst);
	return dst.right;
}

float
InfoDisplay::DrawDigit(float offset, uint32 digitIndex, BRect digitSize,
	const BBitmap *map)
{
	return BlitDigit(this, BPoint(offset, 0), digitIndex, digitSize, map,
		Parent()->ViewColor());
}

TrackDisplay::TrackDisplay(BPoint origin, TrackState *trackState)
	:	InfoDisplay(BRect(kTrackDisplayRect.left + origin.x,
					kTrackDisplayRect.top + origin.y,
					kTrackDisplayRect.right + origin.x,
					kTrackDisplayRect.bottom + origin.y
					), ""),
		trackState(trackState)
{
	segments = new BBitmap(BRect(0, 0, 96 - 1, 16 - 1), B_COLOR_8_BIT);
	segments->SetBits(LCDLarge96x16_raw, 96*16, 0, B_COLOR_8_BIT);
}

TrackDisplay::~TrackDisplay()
{
	delete segments;
}

void
TrackDisplay::Draw(BRect)
{
	// draw the track - first erase background
	SetHighColor(Parent()->ViewColor());
	SetViewColor(Parent()->ViewColor());
	FillRect(Bounds());
	
	// get the track number from the CD engine
	int32 trackNum = trackState->GetTrack();
	float offset = DrawDigit(0, trackNum >= 0 ? trackNum / 10 + 1 : 0,
		kTrackDigitRect, segments);
	DrawDigit(offset + 4, trackNum >= 0 ? trackNum % 10 + 1 : 0,
		kTrackDigitRect, segments);
}

void 
TrackDisplay::AttachedToWindow()
{
	StartObserving(trackState);
}

TimeDisplay::TimeDisplay(BPoint origin, TimeState *timeState)
	:	InfoDisplay(BRect(kTimeDisplayRect.left + origin.x,
					kTimeDisplayRect.top + origin.y,
					kTimeDisplayRect.right + origin.x,
					kTimeDisplayRect.bottom + origin.y
					), ""),		
		timeState(timeState)
{
	segments = new BBitmap(BRect(0, 0, 64 - 1, 9 - 1), B_COLOR_8_BIT);
	segments->SetBits(LCDMedium64x9_raw, 64*9, 0, B_COLOR_8_BIT);
}


TimeDisplay::~TimeDisplay()
{
	delete segments;
}

void 
TimeDisplay::AttachedToWindow()
{
	StartObserving(timeState);
}

void 
TimeDisplay::Draw(BRect)
{
	// erase background
	SetHighColor(Parent()->ViewColor());
	SetViewColor(Parent()->ViewColor());
	FillRect(Bounds());

	// get the time from the CD engine
	int32 minutes, seconds;
	timeState->GetTime(minutes, seconds);
	
	float offset = DrawDigit(0, minutes >= 0 ? minutes / 10 + 1 : 0,
		kTimeDigitRect, segments);
	offset = DrawDigit(offset + 2, minutes >= 0 ? minutes % 10 + 1 : 0,
		kTimeDigitRect, segments);
		
	// draw :
	offset = DrawDigit(offset + 2, 11, kTimeDigitRect, segments);
	
	offset = DrawDigit(offset + 1, seconds >= 0 ? seconds / 10 + 1 : 0,
		kTimeDigitRect, segments);
	offset = DrawDigit(offset + 2, seconds >= 0 ? seconds % 10 + 1 : 0,
		kTimeDigitRect, segments);
}


inline uchar
ShiftComponent(uchar component, float percent)
{
	// change the color by <percent>, make sure we aren't rounding
	// off significant bits
	if (percent >= 1)
		return (uchar)(component * (2 - percent));
	else
		return (uchar)(255 - percent * (255 - component));
}

rgb_color
ShiftColor(rgb_color color, float percent)
{
	rgb_color result = {
		ShiftComponent(color.red, percent),
		ShiftComponent(color.green, percent),
		ShiftComponent(color.blue, percent),
		0
		};
	
	return result;
}
