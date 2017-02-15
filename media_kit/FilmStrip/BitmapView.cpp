#include "BitmapView.h"

BitmapView::BitmapView(BRect bounds,BBitmap* image)
	: BView(bounds,"BitmapView",B_FOLLOW_ALL,B_WILL_DRAW),
		mImage(image)
{
	SetViewColor(0,0,0);
}

BitmapView::~BitmapView(void)
{
}

void BitmapView::SetImage(BBitmap* image)
{ 
	mImage = image;
	if (LockLooper()) {
		// LockLooper() is only necessary if SetImage()
		// is called from a thread other than the
		// parent window's. That's not the case in this
		// app right now, but it could be.
		if (mImage != NULL) {
			Draw(Bounds());
		}
		else {
			Invalidate();
		}
		UnlockLooper();
	}
}

void BitmapView::Draw(BRect updateRect)
{
	if (mImage != NULL) {
		DrawBitmap(mImage,updateRect,updateRect);
	}
}