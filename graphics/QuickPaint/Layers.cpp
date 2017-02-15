/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <stdio.h>

#include "PaintApp.h"
#include "Layers.h"
#include "BitmapDocument.h"
#include "BitmapView.h"

class InputRedirector : public BMessageFilter
{
	public:

								InputRedirector()
									: BMessageFilter(B_ANY_DELIVERY,B_ANY_SOURCE) {};
		virtual filter_result	Filter(BMessage *msg, BHandler **target)
								{
									BView *v;
									if ((msg->what == B_MOUSE_DOWN) ||
										(msg->what == B_MOUSE_UP) ||
										(msg->what == B_MOUSE_MOVED))
									{
										v=dynamic_cast<BView*>(*target);
										if (v) {
											BPoint p = msg->FindPoint("be:view_where");
											p += v->Frame().LeftTop();
											*target = v->Parent();
											if (*target) {
												p += v->Parent()->Bounds().LeftTop();
												msg->ReplacePoint("be:view_where",p);
												// If there is no 'transit' field, this
												// will fail silently... perfect. 
												msg->ReplaceInt32("transit",B_INSIDE_VIEW);
												return B_DISPATCH_MESSAGE;
											};
											return B_SKIP_MESSAGE;
										};
									};
									return B_DISPATCH_MESSAGE;
								};
};

BLayerView::BLayerView(BRect r, int32 xSize, int32 ySize, BBitmapDocument *doc, BHandler *handler)
	: BMatrix(r,"layers",1,doc->Layers(),false,xSize,ySize,B_FOLLOW_ALL,B_DRAW_ON_CHILDREN)
{
	m_document = doc;
	m_handler = handler;
	m_trackingState = -1;
	m_mask = 0xFFFFFFFFFFFFFFFFLL;
	for (int32 i=0;i<doc->Layers();i++) {
		LayerButton *l = new LayerButton;
		l->showing = NULL;
		BRect r = ItemRect(i);
		r.InsetBy(5,5);
		l->disp = new BBitmapView(r,B_FOLLOW_NONE,B_WILL_DRAW,doc);
		l->disp->SetLayerMask(((uint64)1)<<i);
		m_layers.AddItem(l);
		l->disp->SetOptions(NO_SQUARES);
		AddChild(l->disp);
		l->disp->AddFilter(new InputRedirector());
	};
	LayoutViews();
	m_document->AddEditor(this,LAYERS_CHANGED);
}

void BLayerView::ShowLayer(int32 layer)
{
	if ((m_mask >> layer) & 1) return;
	m_mask |= ((uint64)1)<<layer;
	BMessage msg(bmsgLayerMask);
	msg.AddInt64("mask",(int64)m_mask);
	if (m_handler) m_handler->Looper()->PostMessage(&msg,m_handler);
	Invalidate(ItemRect(layer));
};

void BLayerView::HideLayer(int32 layer)
{
	if (!((m_mask >> layer) & 1)) return;
	m_mask &= ~(((uint64)1)<<layer);
	BMessage msg(bmsgLayerMask);
	msg.AddInt64("mask",(int64)m_mask);
	if (m_handler) m_handler->Looper()->PostMessage(&msg,m_handler);
	Invalidate(ItemRect(layer));
};

void BLayerView::CellClicked(BPoint pt, int32 item, BRect itemRect)
{
	if (Window()->CurrentMessage()->FindInt32("buttons") & B_PRIMARY_MOUSE_BUTTON) {
		BMatrix::CellClicked(pt,item,itemRect);
		SetMouseEventMask(B_POINTER_EVENTS,B_LOCK_WINDOW_FOCUS);
		m_trackingState = item;
	} else {
		if ((m_mask >> item) & 1)	HideLayer(item);
		else						ShowLayer(item);
	};
};

void BLayerView::MouseUp(BPoint /* point */)
{
	m_trackingState = -1;
}

void BLayerView::MouseMoved(BPoint point, uint32 transit, const BMessage * /* message */)
{
	int32 thisItem = -1;
	if (m_trackingState >= 0) {
		if (transit == B_INSIDE_VIEW) {
			thisItem = ItemAt(point);
			if ((thisItem >= 0) && (thisItem != m_trackingState)) {
				m_document->MoveLayer(m_trackingState,thisItem);
				m_trackingState = thisItem;
			};
		};
	};
}

void BLayerView::FrameResized(float /* x */, float /* y */)
{
//	LayoutViews();
}

void BLayerView::AttachedToWindow()
{
	BMatrix::AttachedToWindow();
	LayoutViews();
	BRect r = ItemRect(0);
	r.OffsetTo(B_ORIGIN);
	r.InsetBy(5,5);
	m_cross = new BPicture();
	PushState();
		BeginPicture(m_cross);
		SetPenSize(8);
		StrokeLine(r.LeftTop(),r.RightBottom());
		StrokeLine(r.LeftBottom(),r.RightTop());
		EndPicture();
	PopState();
}

BLayerView::~BLayerView()
{
	m_document->AddEditor(this);
}

void BLayerView::AddLayer()
{
	m_document->AddLayer();
}

void BLayerView::DeleteLayer()
{
	m_document->DeleteLayer(m_selection);
}

void BLayerView::DrawAfterChildren(BRect /* updateRect */)
{
	BRect r;
	SetHighColor(255,0,0,150);
	SetDrawingMode(B_OP_ALPHA);
	for (int32 i=0;i<m_layers.CountItems();i++) {
		if (!((m_mask >> i) & 1)) {
			r = ItemRect(i);
			ClipToPicture(m_cross,r.LeftTop());
			FillRect(r);
		};
	};
}

void BLayerView::Select(int newSelection)
{
	BMatrix::Select(newSelection);
	
	BMessage msg(bmsgCurrentLayer);
	msg.AddInt32("layer",newSelection);
	if (m_handler) m_handler->Looper()->PostMessage(&msg,m_handler);
}

void BLayerView::SetHandler(BHandler *handler)
{
	m_handler = handler;
}

void BLayerView::MessageReceived(BMessage *msg)
{
	LayerButton *l;
	BRect b,r,tmpR;
	int32 layer,newLoc;
	uint64 oldMask;
	switch (msg->what) {
		case bmsgLayerAdded:
			layer = msg->FindInt32("layer");
			SetYSlots(m_layers.CountItems()+1);
			l = new LayerButton;
			l->showing = NULL;
			r = ItemRect(layer);
			r.InsetBy(5,5);
			l->disp = new BBitmapView(r,B_FOLLOW_NONE,B_WILL_DRAW,m_document);
			l->disp->SetLayerMask(((uint64)1)<<layer);
			m_layers.AddItem(l,layer);
			if (layer <= m_selection) m_selection++;
			l->disp->SetOptions(NO_SQUARES);
			AddChild(l->disp);
			l->disp->AddFilter(new InputRedirector());
			LayoutViews();
			oldMask = m_mask;
			m_mask = 0;
			for (int32 i=0;i<layer;i++)
				m_mask |= (((oldMask>>i)&1)<<i);
			m_mask |= ((uint64)1)<<layer;
			for (int32 i=layer+1;i<64;i++)
				m_mask |= (((oldMask>>(i-1))&1)<<i);
			r = ItemRect(layer);
			r.bottom = Bounds().bottom;

			{
				BMessage msg(bmsgLayerMask);
				msg.AddInt64("mask",(int64)m_mask);
				msg.AddInt32("force",1);
				if (m_handler) m_handler->Looper()->PostMessage(&msg,m_handler);
			}

			Invalidate(r);
			break;
		case bmsgLayerDeleted:
			layer = msg->FindInt32("layer");
			b = ItemRect(layer);
			r = ItemRect(m_layers.CountItems()-1);
			SetYSlots(m_layers.CountItems()-1);
			l = (LayerButton*)m_layers.ItemAt(m_layers.CountItems()-1);
			l->disp->RemoveSelf();
			delete l->disp;
			m_layers.RemoveItem(l);
			delete l;
			oldMask = m_mask;
			m_mask = 0;
			for (int32 i=0;i<layer;i++)
				m_mask |= (((oldMask>>i)&1)<<i);
			for (int32 i=layer;i<64;i++)
				m_mask |= (((oldMask>>i)&1)<<(i-1));

			{
				BMessage msg(bmsgLayerMask);
				msg.AddInt64("mask",(int64)m_mask);
				msg.AddInt32("force",1);
				if (m_handler) m_handler->Looper()->PostMessage(&msg,m_handler);
			}

//			r.bottom = Bounds().bottom;
//			LayoutViews();
			b.bottom = r.top-1;
			tmpR = b;
			tmpR.OffsetBy(0,m_ySize);
			CopyBits(tmpR,b);
			if (layer < m_selection) Select(m_selection-1);
			else if (layer == m_selection) Invalidate(ItemRect(m_selection));
			Invalidate(r);
			break;
		case bmsgLayersReordered:
			layer = msg->FindInt32("layer");
			newLoc = msg->FindInt32("newLoc");
			l = (LayerButton*)m_layers.RemoveItem(layer);
			m_layers.AddItem(l,newLoc);
			oldMask = m_mask;
			m_mask = 0;
			
			if (newLoc < layer) {
				for (int32 i=0;i<newLoc;i++)
					m_mask |= (((oldMask>>i)&1)<<i);
				m_mask |= (((oldMask>>layer)&1)<<newLoc);
				for (int32 i=newLoc+1;i<=layer;i++)
					m_mask |= (((oldMask>>(i-1))&1)<<i);
				for (int32 i=layer+1;i<64;i++)
					m_mask |= (((oldMask>>i)&1)<<i);
			} else {
				for (int32 i=0;i<layer;i++)
					m_mask |= (((oldMask>>i)&1)<<i);
				for (int32 i=layer;i<newLoc;i++)
					m_mask |= (((oldMask>>(i+1))&1)<<i);
				m_mask |= (((oldMask>>layer)&1)<<newLoc);
				for (int32 i=newLoc+1;i<=64;i++)
					m_mask |= (((oldMask>>i)&1)<<i);
			};

			{
				BMessage msg(bmsgLayerMask);
				msg.AddInt64("mask",(int64)m_mask);
				msg.AddInt32("force",1);
				if (m_handler) m_handler->Looper()->PostMessage(&msg,m_handler);
			}
			
			if (m_selection == layer) {
				m_selection = newLoc;
				goto sendNew;
			} else if ((m_selection > layer) && (m_selection < newLoc)) {
				m_selection--;
				sendNew:
				BMessage msg(bmsgCurrentLayer);
				msg.AddInt32("layer",m_selection);
				if (m_handler) m_handler->Looper()->PostMessage(&msg,m_handler);
			};

			if (newLoc < layer) {
				r = ItemRect(newLoc);
				r.bottom = ItemRect(layer).top-1;
				b = r;
				b.OffsetBy(0,m_ySize);
			} else {
				r = ItemRect(newLoc);
				r.top = ItemRect(layer).bottom+1;
				b = r;
				b.OffsetBy(0,-m_ySize);
			};
			
			CopyBits(r,b);
			Invalidate(ItemRect(newLoc));
			break;
		default:
			BMatrix::MessageReceived(msg);
	};
}

void BLayerView::LayoutViews()
{
	BPoint ul = Bounds().LeftTop();
	BRect r,b;
	float w,h,scale,cx,cy,sx,sy;
	b = m_document->Bounds();
	w = b.IntegerWidth();
	h = b.IntegerHeight();
	Window()->BeginViewTransaction();
	for (int32 i=0;i<m_layers.CountItems();i++) {
		LayerButton *l = (LayerButton*)m_layers.ItemAt(i);
		r = b = ItemRect(i);
		b.InsetBy(5,5);
		if (w > h)	scale = b.IntegerWidth() / w;
		else		scale = b.IntegerHeight() / h;
		l->disp->SetLayerMask(((uint64)1)<<i);
		l->disp->SetScale(scale);
		l->disp->GetMaxSize(&sx,&sy);
		cx = floor(((r.right + r.left)/2) - sx/2);
		cy = floor(((r.bottom + r.top)/2) - sy/2);
		if (l->disp->Frame().top != cy) {
			l->disp->MoveTo(cx-ul.x,cy-ul.y);
			l->disp->ResizeTo(sx,sy);
		};
	};
	Window()->EndViewTransaction();
}

