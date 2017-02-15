/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <stdio.h>
#include <string.h>

#include <Window.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <NodeInfo.h>
#include <TranslationUtils.h>
#include <Screen.h>
#include <ScrollBar.h>

#include "main.h"
#include "iview.h"

TImageView :: TImageView(BRect frame, const char *name, float nscale, int32 nscale_mode) : BView(frame, "imageView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW|B_FRAME_EVENTS)
{
	// This catches the menubar that's also in the window
	// Checking the Frame() elsewhere didn't work.
	skip = frame.LeftTop();

	image = BTranslationUtils::GetBitmapFile(name);
	if(image==NULL){
		printf("Image load failed.\n");
	}

	// Initialize the values to something safe, as SetImageScale might not set them
	scale = 1.0;
	scale_mode = SCALE_NORMAL;
	SetImageScale(nscale, nscale_mode);
}

void TImageView :: AttachedToWindow(void)
{
	FixupScrollbars();
	SetViewColor(B_TRANSPARENT_32_BIT);
}

void TImageView::FrameResized(float width, float height)
{
	FixupScrollbars();
}

void TImageView::SetImageScale(float nscale, int32 nscale_mode)
{

	if(nscale > 0.0) scale = nscale;
	if(nscale_mode < SCALE_LAST) scale_mode = nscale_mode;

	BScreen *scr = new BScreen(Window());
	BRect screenbounds = scr->Frame();
	BRect imagebounds = image->Bounds();
	delete scr;

	if(scale_mode == SCALE_MAX){
		scale = 1.0; // For safety's sake set scale to something reasonable
		scale_mode = SCALE_NORMAL;
		float t = (screenbounds.Width()- B_V_SCROLL_BAR_WIDTH - skip.x - CRUFT_SIZE_X) / floor((imagebounds.right+1) - 0.5);
		scale = t;
		t = (screenbounds.Height()- B_H_SCROLL_BAR_HEIGHT - skip.y - CRUFT_SIZE_Y) / floor((imagebounds.bottom+1) - 0.5);
		if(t < scale) scale = t;
	}
	if(scale_mode == SCALE_NORMAL){
		SetFlags(Flags() & ~B_FULL_UPDATE_ON_RESIZE);
	}else{
		SetFlags(Flags() | B_FULL_UPDATE_ON_RESIZE);
	}
	FixupScrollbars();
	Invalidate();
}

void TImageView::GetMaxSize(float *maxx, float *maxy)
{
	BWindow *mywin = Window();
	float mx, my;
	mx = my = 0.0;
	if(mywin && mywin->Lock()){
		BRect imagebounds = image->Bounds();
		if(scale_mode == SCALE_NORMAL){
			mx = floor((imagebounds.right)*scale);
			my = floor((imagebounds.bottom)*scale);
		} else {
			if(scale_mode != SCALE_TOFIT) {
				fprintf(stderr, "[%s:%d] scale_mode set to strange value (%ld)\n",__FILE__,__LINE__,scale_mode);
			}
			// Signal error condition to caller
			mx = -1.0;
			my = -1.0;
		} 
		mywin->Unlock();
	}
	if(maxx) *maxx = mx;
	if(maxy) *maxy = my;
}

void TImageView::FixupScrollbars(void)
{
	BRect bounds;
	BScrollBar *sb;

	bounds = Bounds();
	float myPixelWidth = bounds.Width();
	float myPixelHeight = bounds.Height();
	float maxWidth = 1,maxHeight = 1;

	if(image!=NULL){
		// get max size of image
		GetMaxSize(&maxWidth, &maxHeight);
	} else fprintf(stderr, "Image is null\n");
		
	float propW = myPixelWidth/maxWidth;
	float propH = myPixelHeight/maxHeight;
	
	float rangeW = maxWidth - myPixelWidth;
	float rangeH = maxHeight - myPixelHeight;
	if(rangeW < 0) rangeW = 0;
	if(rangeH < 0) rangeH = 0;


	if ((sb=ScrollBar(B_HORIZONTAL))!=NULL) {
		sb->SetProportion(propW);
		sb->SetRange(0,rangeW);
		// Steps are 1/8 visible window for small steps
		//   and 1/2 visible window for large steps
		sb->SetSteps(myPixelWidth / 8.0, myPixelWidth / 2.0);
	} 

	if ((sb=ScrollBar(B_VERTICAL))!=NULL) {
		sb->SetProportion(propH);
		sb->SetRange(0,rangeH);
		// Steps are 1/8 visible window for small steps
		//   and 1/2 visible window for large steps
		sb->SetSteps(myPixelHeight / 8.0, myPixelHeight / 2.0);
	}
}

void TImageView :: Draw(BRect invalid)
{
	BRect source = invalid;
	if(image){
		float scale_x = scale;
		float scale_y = scale;
		if(scale_mode != SCALE_NORMAL){
			// We're scaling to fit the window
			// We need to find the proper scales
			BRect rect = image->Bounds();
			BRect b = Bounds();
			scale_x = b.Width() / floor((rect.right+1) - 0.5);
			scale_y = b.Height() / floor((rect.bottom+1) - 0.5);
		}
		source.top /= scale_y;
		source.bottom /= scale_y;
		source.right /= scale_x;
		source.left /= scale_x;
		DrawBitmap(image, source, invalid);
	}
}

