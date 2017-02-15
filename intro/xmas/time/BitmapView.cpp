/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Application.h>
#include <Bitmap.h>
#include <stdio.h>

#include "BitmapView.h"

BitmapView::BitmapView(BBitmap* bitmap, BRect frame, const char* name,
	uint32 resizingMode, uint32 flags)
: BView(frame, name, resizingMode, flags), m_pBitmap(bitmap)
{ }

BitmapView::~BitmapView()
{
	if (m_pBitmap)
		delete m_pBitmap;
}

void BitmapView::AttachedToWindow()
{
	BView* parent = Parent();
	if (parent) {
		// set view color so transparency appears
		// in parent view color
		SetViewColor(parent->ViewColor());
	}
}

void BitmapView::Draw(BRect updateRect)
{
	if (m_pBitmap) {
		SetDrawingMode(B_OP_OVER);
		DrawBitmap(m_pBitmap, updateRect, updateRect);
	}
}

void BitmapView::ResizeToPreferred()
{
	float width, height;
	GetPreferredSize(&width, &height);
	ResizeTo(width, height);
}

void BitmapView::GetPreferredSize(float* width, float* height)
{
	if (m_pBitmap) {
		BRect bounds = m_pBitmap->Bounds();
		*width = bounds.Width();
		*height = bounds.Height();
	} else {
		*width = 0; *height = 0;
	}
}

BBitmap* BitmapView::Bitmap()
{
	return m_pBitmap;
}

void BitmapView::SetBitmap(BBitmap* bitmap)
{
	if (m_pBitmap) {
		delete m_pBitmap;
	}
	m_pBitmap = bitmap;
}
