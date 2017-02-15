/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "PaintApp.h"
#include "BitmapDocument.h"
#include "BitmapView.h"
#include "Toolbox.h"

BBitmapView::BBitmapView(BRect rect, ulong resizeMode,
	ulong flags, BBitmapDocument *bitmap)
	: BView(rect,"BitmapView",resizeMode,flags)
{
	m_bitmap = bitmap;
	m_bitmap->AddEditor(this,BITMAP_CHANGED);
	m_layerMask = 0xFFFFFFFFFFFFFFFFLL;
	m_options = 0;
	SetScale(1);
};

void BBitmapView::SetLayerMask(uint64 layerMask, int32 force)
{
	if (!force && (m_layerMask == layerMask)) return;

	m_layerMask = layerMask;
	Invalidate();
};

void BBitmapView::GetMaxSize(float *width, float *height)
{
	BRect r = m_bitmap->Bounds();
	*width = floor((r.right+1)*m_scale - 0.5);
	*height = floor((r.bottom+1)*m_scale - 0.5);
};

void BBitmapView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case bmsgLayerMask:
		{
			uint64 mask;
			msg->FindInt64("mask",(int64*)&mask);
			SetLayerMask(mask,msg->FindInt32("force"));
			break;
		};
		case bmsgBitmapDirty:
		{
			BRect r;
			int iter=0;
			int32 layer = msg->FindInt32("layer");
			if ((m_layerMask>>layer) & 1) {
				while (msg->FindRect("rect",iter++,&r) == B_OK) {
					r.left = floor(r.left * m_scale);
					r.top = floor(r.top * m_scale);
					r.right = ceil((r.right+1) * m_scale)-1;
					r.bottom = ceil((r.bottom+1) * m_scale)-1;
					Invalidate(r);
				};
			};
			break;
		};
		default:
			BView::MessageReceived(msg);
	};
};

BBitmapView::~BBitmapView()
{
	m_bitmap->RemoveEditor(this);
};

void BBitmapView::SetScale(float scale)
{
	if (m_scale == scale) return;
	m_scale = scale;
	FixupScrollbars();
	Invalidate();
};

float BBitmapView::Scale()
{
	return m_scale;
};

void BBitmapView::FrameResized(float /* width */, float /* height */)
{
	FixupScrollbars();	
};

void BBitmapView::AttachedToWindow()
{
	FixupScrollbars();
	SetViewColor(B_TRANSPARENT_COLOR);
};

void BBitmapView::Draw(BRect updateRect)
{
	BBitmap *b = m_bitmap->RealBitmap();
	BRect bnd = b->Bounds();
	BRect bnds(0,0,(bnd.right+1)*m_scale-1,(bnd.bottom+1)*m_scale-1);

	BRect onBitmap(
		floor(updateRect.left/m_scale),floor(updateRect.top/m_scale),
		ceil((updateRect.right+1)/m_scale-1),ceil((updateRect.bottom+1)/m_scale-1));

	m_bitmap->ReadLock();
	m_bitmap->Compose(m_layerMask,this,bnds,onBitmap,m_options);
	m_bitmap->ReadUnlock();
	
	Sync();
};

void BBitmapView::FixupScrollbars()
{
	BRect bounds = Bounds();
	BScrollBar *sb;
	
	float myPixelWidth = bounds.Width();
	float myPixelHeight = bounds.Height();
	float maxWidth,maxHeight;
	GetMaxSize(&maxWidth,&maxHeight);
		
	float propW = myPixelWidth/maxWidth;
	float propH = myPixelHeight/maxHeight;
	
	float rangeW = maxWidth - myPixelWidth;
	float rangeH = maxHeight - myPixelHeight;
	
	sb = ScrollBar(B_HORIZONTAL);
	if (sb) {
		sb->SetProportion(propW);
		sb->SetRange(0,max(0,rangeW));
	};
	sb = ScrollBar(B_VERTICAL);
	if (sb) {
		sb->SetProportion(propH);
		sb->SetRange(0,max(0,rangeH));
	};
};

void BBitmapView::SetOptions(uint32 options)
{
	m_options = options;
	Invalidate();
}

