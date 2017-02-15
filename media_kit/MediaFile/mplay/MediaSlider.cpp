#include "MediaSlider.h"
#include "MediaView.h"
#include <Bitmap.h>
#include <stdio.h>


const float		kSliderHeight = 16.0;
const float		kThumbWidth = 6.0;
const rgb_color	kWhite = {255, 255, 255, 255};
const rgb_color	kBlack = {0, 0, 0, 255};
const rgb_color	kSliderBackgroundColor = {173, 223, 165, 255};
const rgb_color	kSliderInsetDarkColor = {121, 157, 116, 255};
const rgb_color	kSliderInsetLightColor = {206, 236, 201, 255};
const rgb_color	kThumbBackgroundColor = {255, 0, 0, 255};
const rgb_color	kThumbDarkColor = {181, 0, 0, 255};
const rgb_color	kThumbLightColor = {255, 154, 156, 255};


_MediaSlider_::_MediaSlider_(
	BRect		frame, 
	MediaView	*owner)
		: BView(frame, B_EMPTY_STRING, B_FOLLOW_BOTTOM | B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW | B_FRAME_EVENTS)
{
	fOwner = owner;
	fBitmap = NULL;
	fOffscreenView = NULL;
	fTotalTime = 1;
	fCurTime = 0;
	fThumbRect.Set(2.0, 2.0, 2.0 + kThumbWidth - 1.0, kSliderHeight - 1.0 - 1.0);
	fMouseDown = false;

	ResetBitmap();
}


_MediaSlider_::~_MediaSlider_()
{
	delete (fBitmap);
	//delete (fOffscreenView) is done for you implicitly
}


void
_MediaSlider_::SetTotalTime(
	bigtime_t	totalTime)
{
	fTotalTime = totalTime;
}


void
_MediaSlider_::SetCurTime(
	bigtime_t	curTime)
{
	fCurTime = curTime;
	UpdateThumb(false);
}


void
_MediaSlider_::AttachedToWindow()
{
	SetViewColor(Parent()->ViewColor());
}


void
_MediaSlider_::Draw(
	BRect	updateRect)
{
	DrawBitmap(fBitmap, BPoint(0.0, (Bounds().Height() - kSliderHeight) / 2));
}


void
_MediaSlider_::MouseDown(
	BPoint	where)
{
	fOwner->fScrubTime = PointToTime(where);
	fOwner->fScrubSem = create_sem(1, "MediaView::fScrubSem");

	if (!fOwner->fPlaying) {
		release_sem(fOwner->fPlaySem);
		acquire_sem(fOwner->fPlaySem);
	}

	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);

	fMouseDown = true;
}


void
_MediaSlider_::MouseMoved(
	BPoint			where, 
	uint32			code, 
	const BMessage	*message)
{
	if (!fMouseDown)
		return;

	bigtime_t scrubTime = PointToTime(where);
	
	if (scrubTime == fOwner->fScrubTime)
		return;

	fOwner->fScrubTime = scrubTime;
	release_sem(fOwner->fScrubSem);	
}


void
_MediaSlider_::MouseUp(
	BPoint	where)
{
	delete_sem(fOwner->fScrubSem);
	fOwner->fScrubSem = B_ERROR;

	fMouseDown = false;
}


void
_MediaSlider_::FrameResized(
	float	width,
	float	height)
{
	ResetBitmap();
	Draw(Bounds());
}


bigtime_t
_MediaSlider_::PointToTime(
	BPoint	where)
{
	BRect bounds = Bounds();

	where.x = (where.x < 0.0) ? 0.0 : where.x;
	where.x = (where.x > bounds.right) ? bounds.right : where.x;

	return ((int64)(((float)where.x / (float)bounds.Width()) * fTotalTime));
}


void
_MediaSlider_::UpdateThumb(
	bool	force)
{
	fBitmap->Lock();

	BRect bounds = fOffscreenView->Bounds();

	bounds.left += 2.0;
	bounds.right -= kThumbWidth + 3.0;

	BRect thumbRect = fThumbRect;
	thumbRect.OffsetTo(bounds.left + (bounds.right * ((float)fCurTime / (float)fTotalTime)), thumbRect.top);

	if ((!force) && (thumbRect == fThumbRect)) {
		fBitmap->Unlock();
		return;
	}

	fOffscreenView->SetLowColor(kSliderBackgroundColor);
	fOffscreenView->FillRect(fThumbRect, B_SOLID_LOW);

	BRect drawRect = fThumbRect | thumbRect;
	fThumbRect = thumbRect;

	fOffscreenView->SetHighColor(kBlack);
	fOffscreenView->StrokeRect(thumbRect, B_SOLID_HIGH);
	thumbRect.InsetBy(1.0, 1.0);

	fOffscreenView->BeginLineArray(4);
	fOffscreenView->AddLine(thumbRect.RightTop(), thumbRect.RightBottom(), kThumbDarkColor);
	fOffscreenView->AddLine(thumbRect.RightBottom(), thumbRect.LeftBottom(), kThumbDarkColor);
	fOffscreenView->AddLine(thumbRect.LeftBottom(), thumbRect.LeftTop(), kThumbLightColor);
	fOffscreenView->AddLine(thumbRect.LeftTop(), thumbRect.RightTop(), kThumbLightColor);
	fOffscreenView->EndLineArray();	
	thumbRect.InsetBy(1.0, 1.0);

	fOffscreenView->SetHighColor(kThumbBackgroundColor);
	fOffscreenView->FillRect(thumbRect, B_SOLID_HIGH);

	fOffscreenView->Sync();
	fBitmap->Unlock();

	BPoint	drawLoc(0.0, (Bounds().Height() - kSliderHeight) / 2);
	BRect	dstRect = drawRect;
	dstRect.OffsetBy(drawLoc);
	DrawBitmap(fBitmap, drawRect, dstRect);
}


void
_MediaSlider_::ResetBitmap()
{
	delete (fBitmap);
	//delete (fOffscreenView) is done for you implicitly

	BRect bounds = Bounds();
	bounds.bottom = bounds.top + kSliderHeight - 1.0;

	fBitmap = new BBitmap(bounds, B_CMAP8, true);
	fOffscreenView = new BView(bounds, B_EMPTY_STRING, 0, 0);		
	
	fBitmap->Lock();
	fBitmap->AddChild(fOffscreenView);

	rgb_color viewColor = ViewColor();
	rgb_color dark = tint_color(viewColor, B_DARKEN_1_TINT);

	fOffscreenView->BeginLineArray(6);
	fOffscreenView->AddLine(bounds.LeftBottom(), bounds.LeftTop(), dark);
	fOffscreenView->AddLine(bounds.LeftTop(), bounds.RightTop(), dark);
	fOffscreenView->AddLine(bounds.RightTop(), bounds.RightBottom(), kWhite);
	fOffscreenView->AddLine(bounds.RightBottom(), bounds.LeftBottom(), kWhite);

	bounds.InsetBy(1.0, 1.0);

	fOffscreenView->AddLine(bounds.LeftBottom(), bounds.LeftTop(), kBlack);
	fOffscreenView->AddLine(bounds.LeftTop(), bounds.RightTop(), kBlack);

	fOffscreenView->EndLineArray();		

	bounds.left++;
	bounds.top++;

	fOffscreenView->SetLowColor(kSliderBackgroundColor);
	fOffscreenView->FillRect(bounds, B_SOLID_LOW);

/*
	int32	numInsets = floor(bounds.Width() / 6);
	BRect	insetRect = bounds;
	insetRect.InsetBy(0.0, 3.0);
	insetRect.right = insetRect.left;

	fOffscreenView->BeginLineArray(numInsets * 2);

	for (int32 i = 0; i < numInsets; i++) {
		fOffscreenView->AddLine(insetRect.LeftTop(), insetRect.LeftBottom(), kSliderInsetDarkColor);
		insetRect.OffsetBy(1.0, 0.0);
		fOffscreenView->AddLine(insetRect.LeftTop(), insetRect.LeftBottom(), kSliderInsetLightColor);

		insetRect.OffsetBy(5.0, 0.0);
	}

	fOffscreenView->EndLineArray();		
*/

	fOffscreenView->Sync();
	fBitmap->Unlock();

	UpdateThumb(true);
}