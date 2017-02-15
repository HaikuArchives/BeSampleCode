/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Bitmap.h>
#include <stdio.h>
#include "colorutils.h"
#include "offscreenview.h"

OffscreenView::OffscreenView(BRect r)
	: BView(r, "OffscreenView", B_FOLLOW_NONE, 0), m_pb(0)
{
	MoveTo(0,0);
	m_pb = new BBitmap(Bounds(), B_RGBA32_HOST, true);
	m_pb->AddChild(this);
}

OffscreenView::~OffscreenView()
{
	m_pb->RemoveChild(this);
	delete m_pb;
}

status_t OffscreenView::Begin()
{
	if (! m_pb->Lock()) {
		return B_ERROR;
	}
	
	return B_OK;
}

BBitmap* OffscreenView::End()
{
	Sync();
	m_pb->Unlock();
	return m_pb;
}

void OffscreenView::FrameResized(float width, float height)
{
	printf("offscreen view: frame resized\n");
}

