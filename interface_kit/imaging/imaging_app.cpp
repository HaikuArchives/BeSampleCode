/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "imaging_app.h"
#include "GMCGraphics.h"
#include <Bitmap.h>

#include "scale.h"

//==================================================================
BTSImagingView::BTSImagingView(BRect frame)
	: BView(frame, "imaging_view", B_FOLLOW_NONE, B_WILL_DRAW)
{
}

void
BTSImagingView::AttachedToWindow()
{
	// Setup the bitmap for the source image
	BRect sourceRect(0,0,160,120);
	BView *sourceView = new BView(sourceRect, "source", B_FOLLOW_NONE, B_WILL_DRAW);

	//fSourceImage = new BBitmap(sourceRect,B_GRAYSCALE_8_BIT,TRUE);
	fSourceImage = new BBitmap(sourceRect,B_RGB_32_BIT,TRUE);
	fSourceImage->Lock();
	fSourceImage->AddChild(sourceView);
	fSourceImage->Unlock();

	// Draw the graphic into the source image
	AMonthGraphic *monthGraphic = new AMonthGraphic(sourceView);
	monthGraphic->SetSize(BPoint(160,120));
	sourceView->Window()->Lock();
	monthGraphic->Draw(sourceView);
	sourceView->Sync();
	sourceView->Window()->Unlock();
	
	// Create the destination image by scaling the source image
	BRect dstRect(0,0,320,240);
	//fDestinationImage = new BBitmap(dstRect,B_GRAYSCALE_8_BIT);
	fDestinationImage = new BBitmap(dstRect,B_RGB_32_BIT);
	scale(fSourceImage, fDestinationImage, 1.5, 1.5);
	
	// Now create the main offscreen image and view and draw the
	// unaltered source into it.
	fOffscreenView = new BView(Bounds(), "offscreen", B_FOLLOW_NONE, B_WILL_DRAW);
	fOffscreenImage = new BBitmap(Bounds(),B_RGB_32_BIT,TRUE);
	fOffscreenImage->Lock();
	fOffscreenImage->AddChild(fOffscreenView);
	fOffscreenView->DrawBitmap(fSourceImage, sourceRect, sourceRect);
	fOffscreenView->DrawBitmap(fDestinationImage, dstRect, BRect(180,0,
		180+dstRect.IntegerWidth(),dstRect.IntegerHeight()));
	fOffscreenView->DrawBitmap(fSourceImage, sourceRect, BRect(180,200,
		180+240,200+180));
	fOffscreenView->Sync();
	fOffscreenImage->Unlock();

}

void
BTSImagingView::Draw(BRect updateRect)
{
	DrawBitmap(fOffscreenImage, updateRect, updateRect);
}

//==================================================================
BTSImagingWindow::BTSImagingWindow()
	: BWindow(BRect(30,30,630,430),"imaging_window",B_TITLED_WINDOW,
		B_NOT_RESIZABLE|B_NOT_ZOOMABLE)
{
	fMainView = new BTSImagingView(BRect(0,0,600,400));
	
	Lock();
	AddChild(fMainView);
	Unlock();
	
	Show();
}

bool
BTSImagingWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return TRUE;
}

//==================================================================
BTSImagingApp::BTSImagingApp()
	: BApplication("application/x-vnd.Be-DTS.scaler")
{
	fMainWindow = new BTSImagingWindow;
}
