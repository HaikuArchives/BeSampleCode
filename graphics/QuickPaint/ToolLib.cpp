/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <math.h>
#include <stdio.h>
#include "ToolLib.h"
#include "BitmapDocument.h"
#include "BitmapEditor.h"
#include "PaintApp.h"

/***********************************************/

inline BRect Integerize(BRect r)
{
	r.right = floor(r.right+1);
	r.bottom = floor(r.bottom+1);
	r.left = floor(r.left);
	r.top = floor(r.top);
	return r;
};

void BRectBasedTool::DrawIcon(BView *drawInto, BRect location)
{
	int32 w = (int32)location.IntegerWidth();
	int32 h = (int32)location.IntegerHeight();
	w /= 8; h /= 8;
	location.InsetBy(w,h);
	DrawPrimitive(drawInto,location);
};

BRectBasedTool::BRectBasedTool(bool fill) : BTool()
{
	m_fill = fill;
	m_eventOptions |= B_NO_POINTER_HISTORY;
};

void BRectBasedTool::StartUsing()
{
	m_theRect.Set(
		m_oldPoint.x,m_oldPoint.y,
		m_oldPoint.x,m_oldPoint.y);
		
	m_document->Lock();
		m_document->View(m_whichLayer)->SetDrawingMode(B_OP_INVERT);
		m_document->View(m_whichLayer)->SetPenSize(10);
		m_fudge = floor(m_document->View(m_whichLayer)->PenSize() * 0.5 + 0.5);
		DrawPrimitive(m_document->View(m_whichLayer),m_theRect);
	m_document->Unlock();
};

void BRectBasedTool::PointerMoved(BPoint newPoint)
{
	BRegion r;
	BRect oldRect = m_theRect;

	m_document->Lock();
		DrawPrimitive(m_document->View(m_whichLayer),m_theRect);
		if (newPoint.y < m_invokePoint.y) {
			m_theRect.top = newPoint.y;
			m_theRect.bottom = m_invokePoint.y;
		} else {
			m_theRect.top = m_invokePoint.y;
			m_theRect.bottom = newPoint.y;
		};
		if (newPoint.x < m_invokePoint.x) {
			m_theRect.left = newPoint.x;
			m_theRect.right = m_invokePoint.x;
		} else {
			m_theRect.left = m_invokePoint.x;
			m_theRect.right = newPoint.x;
		};
		DrawPrimitive(m_document->View(m_whichLayer),m_theRect);
		m_document->View(m_whichLayer)->Sync();
	m_document->Unlock();

	GetDeltaInval(&r,oldRect,m_theRect);
	m_document->SetDirty(&r,m_whichLayer);
};

void BRectBasedTool::StopUsing()
{
	BRegion r;

	m_document->Lock();
		DrawPrimitive(m_document->View(m_whichLayer),m_theRect);
		m_document->View(m_whichLayer)->SetDrawingMode(B_OP_COPY);
		DrawPrimitive(m_document->View(m_whichLayer),m_theRect);
		m_document->View(m_whichLayer)->Sync();
	m_document->Unlock();

	InvalPrimitive(&r,m_theRect);
	m_document->SetDirty(&r,m_whichLayer);	
};

/***********************************************/

void BRectTool::DrawPrimitive(BView *drawInto, BRect r)
{
	if (m_fill) drawInto->FillRect(r);
	else		drawInto->StrokeRect(r);
};

void BRectTool::GetDeltaInval(BRegion *inval, BRect oldRect, BRect newRect)
{
	BRegion tmpReg;
	if (m_fill)	{
		inval->Set(Integerize(newRect));
		inval->Include(Integerize(oldRect));
	} else {
		BRect tmpRect = oldRect;
		tmpRect.InsetBy(-m_fudge,-m_fudge);
		tmpReg.Set(Integerize(tmpRect));
		tmpRect = oldRect;
		tmpRect.InsetBy(m_fudge+1,m_fudge+1);
		tmpReg.Exclude(Integerize(tmpRect));

		tmpRect = newRect;
		tmpRect.InsetBy(-m_fudge,-m_fudge);
		inval->Set(Integerize(tmpRect));
		tmpRect = newRect;
		tmpRect.InsetBy(m_fudge+1,m_fudge+1);
		inval->Exclude(Integerize(tmpRect));
		inval->Include(&tmpReg);
	};
};

void BRectTool::InvalPrimitive(BRegion *inval, BRect r)
{
	if (m_fill) {
		inval->Set(Integerize(r));
	} else {
		BRect b = r;
		b.InsetBy(m_fudge+1,m_fudge+1);
		r.InsetBy(-m_fudge,-m_fudge);
		inval->Set(Integerize(r));
		inval->Exclude(Integerize(b));
	};
};

/***********************************************/

void BEllipseTool::DrawPrimitive(BView *drawInto, BRect r)
{
	if (m_fill) drawInto->FillEllipse(r);
	else		drawInto->StrokeEllipse(r);
};

void BEllipseTool::GetDeltaInval(BRegion *inval, BRect oldRect, BRect newRect)
{
	if (m_fill) {
		inval->Set(Integerize(oldRect));
		inval->Include(Integerize(newRect));
	} else {
		BRect r = oldRect;
		r.InsetBy(-m_fudge,-m_fudge);
		inval->Set(Integerize(r));
		r = newRect;
		r.InsetBy(-m_fudge,-m_fudge);
		inval->Include(Integerize(r));
	};
};

void BEllipseTool::InvalPrimitive(BRegion *inval, BRect r)
{
	if (m_fill) {
		inval->Set(Integerize(r));
	} else {
		r.InsetBy(-m_fudge,-m_fudge);
		inval->Set(Integerize(r));
	};
};

/***********************************************/

BPoint iconPoints[4] = 
{
	BPoint(0.2,0.2),
	BPoint(0.1,1.3),
	BPoint(0.9,-0.3),
	BPoint(0.8,0.8),
};

BPenTool::BPenTool()
{
	m_fullPixels = true;
}

void BPenTool::DrawIcon(BView *drawInto, BRect location)
{
	BPoint l_iconPoints[4];
	float width = location.Width();
	float height = location.Height();
	for (int32 i=0;i<4;i++) {
		l_iconPoints[i].x = location.left + iconPoints[i].x * width;
		l_iconPoints[i].y = location.top + iconPoints[i].y * height;
	};
	drawInto->SetPenSize(3.0);
	drawInto->StrokeBezier(l_iconPoints);
	drawInto->SetPenSize(1.0);
};

void BPenTool::GenerateBrush()
{
	BRect r(0,0,15,15);
	m_brush = new BBitmap(BRect(0,0,15,15),ARGB_FORMAT);
	m_hotspot.x = m_hotspot.y = 8;
	int32 x = r.IntegerWidth()+1;
	int32 y = r.IntegerHeight()+1;
	float dis,disX,disY,maxDis = 7;
	ARGBPixel pixel,*bits = (ARGBPixel*)m_brush->Bits();
	int32 offs = m_brush->BytesPerRow()/4 - x;
	
	pixel.r = m_foreColor.red;
	pixel.g = m_foreColor.green;
	pixel.b = m_foreColor.blue;
	for (int32 yy=0;yy<y;yy++) {
		for (int32 xx=0;xx<x;xx++) {
			disX = (xx-8)/maxDis;
			disY = (yy-8)/maxDis;
			dis = sqrt(disX*disX + disY*disY);
			if (dis >= 1.0) pixel.a = 0;
			else {
				pixel.a = (uint8)((1.0 - dis) * 255);
			};
			*bits++ = pixel;
		};
		bits += offs;
	};
}

void BPenTool::SetDrawingOp()
{
	m_document->View(m_whichLayer)->SetDrawingMode(B_OP_ALPHA);
	m_document->View(m_whichLayer)->SetBlendingMode(B_PIXEL_ALPHA,B_ALPHA_COMPOSITE);
}

void BPenTool::StartUsing()
{
	GenerateBrush();
	m_document->Lock();
		BRect r = m_brush->Bounds();
		BView *v = m_document->View(m_whichLayer);
		r.OffsetBy(m_invokePoint-m_hotspot);
		SetDrawingOp();
		v->DrawBitmap(m_brush,r);
	m_document->Unlock();
};

void BPenTool::PointerMoved(BPoint newPoint)
{
	BRegion reg;
	BRect combined;

	m_document->Lock();
		BRect r = combined = m_brush->Bounds();
		int32 halfWidth = ((int32)(r.IntegerWidth()+1))/4;
		int32 halfHeight = ((int32)(r.IntegerHeight()+1))/4;
		BView *v = m_document->View(m_whichLayer);
		SetDrawingOp();
		combined.OffsetBy(newPoint-m_hotspot);

		if ((fabs(newPoint.x-m_oldPoint.x) > halfWidth) ||
			(fabs(newPoint.y-m_oldPoint.y) > halfHeight)) {
			BPoint p;
			BRect b;
			int32 px = (int32)m_oldPoint.x;
			int32 py = (int32)m_oldPoint.y;
			int32 iter;
			float rx = fabs(newPoint.x-px) / halfWidth;
			float ry = fabs(newPoint.y-py) / halfHeight;
			if (rx > ry) {
				iter = (int32)floor(rx);
				if ((newPoint.x-px) < 0) halfWidth *= -1;
				for (int32 i=0;i<iter;i++) {
					px += halfWidth;
					p.x = px;
					p.y = floor(py + (((newPoint.y-py) * i) / iter));
					b = r;
					b.OffsetBy(p-m_hotspot);
					v->DrawBitmap(m_brush,b);
					combined = combined|b;
				};
			} else {
				iter = (int32)floor(ry);
				if ((newPoint.y-py) < 0) halfHeight *= -1;
				for (int32 i=0;i<iter;i++) {
					py += halfHeight;
					p.y = py;
					p.x = floor(px + (((newPoint.x-px) * i) / iter));
					b = r;
					b.OffsetBy(p-m_hotspot);
					v->DrawBitmap(m_brush,b);
					combined = combined|b;
				};
			};
		};
		
		r.OffsetBy(newPoint-m_hotspot);
		v->DrawBitmap(m_brush,r);
	m_document->Unlock();
		
	reg.Set(combined);
	m_document->SetDirty(&reg,m_whichLayer);
};

void BPenTool::StopUsing()
{
	delete m_brush;
}

/***********************************************/

void BAlphaTool::DrawIcon(BView *drawInto, BRect location)
{
	drawInto->FillRect(location);
	drawInto->PushState();
		drawInto->SetHighColor(drawInto->LowColor());
		BPenTool::DrawIcon(drawInto,location);
	drawInto->PopState();
};

void BAlphaTool::GenerateBrush()
{
	BRect r(0,0,15,15);
	m_brush = new BBitmap(BRect(0,0,15,15),ARGB_FORMAT);
	m_hotspot.x = m_hotspot.y = 8;
	int32 x = r.IntegerWidth()+1;
	int32 y = r.IntegerHeight()+1;
	float dis,disX,disY,maxDis = 7;
	ARGBPixel pixel, *bits = (ARGBPixel*)m_brush->Bits();
	int32 offs = m_brush->BytesPerRow()/4 - x;
	
	pixel.r = pixel.g = pixel.b = 0;
	for (int32 yy=0;yy<y;yy++) {
		for (int32 xx=0;xx<x;xx++) {
			disX = (xx-8)/maxDis;
			disY = (yy-8)/maxDis;
			dis = sqrt(disX*disX + disY*disY);
			if (dis >= 1.0) pixel.a = 0;
			else {
				pixel.a = (uint8)((1.0 - dis) * 255);
			};
			*bits++ = pixel;
		};
		bits += offs;
	};
}

void BAlphaTool::SetDrawingOp()
{
	m_document->View(m_whichLayer)->SetDrawingMode(B_OP_SUBTRACT);
}

/***********************************************/

BHandTool::BHandTool()
{
	m_eventOptions |= B_NO_POINTER_HISTORY;
	m_fullPixels = true;
}

void BHandTool::StartUsing()
{
	BRect r = m_document->Bounds();
	m_original = new BBitmap(r,ARGB_FORMAT,true);
	m_perverted = new BBitmap(r,ARGB_FORMAT);
	m_drawer = new BView(r,"drawer",B_FOLLOW_ALL,0);
	m_original->Lock();
	m_original->AddChild(m_drawer);
	
	m_document->Lock();
		m_drawer->DrawBitmap(m_document->RealBitmap(m_whichLayer),m_original->Bounds());

		int32 x = r.IntegerWidth()+1;
		int32 y = r.IntegerHeight()+1;
		ARGBPixel pixel,
			*sbits = (ARGBPixel*)m_original->Bits(),
			*dbits = (ARGBPixel*)m_perverted->Bits();
		int32 spoffs = m_original->BytesPerRow()/4 - x;
		int32 dpoffs = m_perverted->BytesPerRow()/4 - x;
		int32 	limitTop=0x7FFFFFFF,limitLeft=0x7FFFFFFF,
				limitBottom=-0x7FFFFFFF,limitRight=-0x7FFFFFFF;

		for (int32 yy=0;yy<y;yy++) {
			for (int32 xx=0;xx<x;xx++) {
				pixel = *sbits++;
				if (pixel.a) {
					if (yy < limitTop) limitTop = yy;
					if (yy > limitBottom) limitBottom = yy;
					if (xx < limitLeft) limitLeft = xx;
					if (xx > limitRight) limitRight = xx;
					pixel.a = pixel.a * 2 / 3;
				};
				*dbits++ = pixel;
			};
			sbits += spoffs;
			dbits += dpoffs;
		};

		BView *v = m_document->View(m_whichLayer);
		v->SetDrawingMode(B_OP_OVER);
		v->DrawBitmap(m_perverted,m_perverted->Bounds());
	m_document->Unlock();

	m_limits.Set(limitLeft,limitTop,limitRight,limitBottom);

	BRegion reg;
	reg.Set(m_limits);
	m_document->SetDirty(&reg,m_whichLayer);
}

void BHandTool::PointerMoved(BPoint newPoint)
{
	BRect src,r;

	m_document->Lock();
		BView *v = m_document->View(m_whichLayer);
		v->SetHighColor(0,0,0,0);
		v->SetDrawingMode(B_OP_OVER);
		r = m_limits;
		r.OffsetBy(m_oldPoint-m_invokePoint);
		v->FillRect(r);
		src = r = m_limits;
		r.OffsetBy(newPoint-m_invokePoint);
		v->DrawBitmap(m_perverted,src,r);
	m_document->Unlock();

	BRegion reg;

	reg.Set(r);
	r = m_limits;
	r.OffsetBy(m_oldPoint-m_invokePoint);
	reg.Include(r);
	m_document->SetDirty(&reg,m_whichLayer);
}

void BHandTool::StopUsing()
{
	m_document->Lock();
		BView *v = m_document->View(m_whichLayer);
		BRect r = m_original->Bounds();
		v->SetHighColor(0,0,0,0);
		v->SetDrawingMode(B_OP_OVER);
		v->FillRect(r);
		BRect src = r = m_limits;
		r.OffsetBy(m_curOnBitmap-m_invokePoint);
		v->DrawBitmap(m_original,src,r);
	m_document->Unlock();

	BRegion reg;

	reg.Set(r);
	r = m_limits;
	r.OffsetBy(m_oldPoint-m_invokePoint);
	reg.Include(r);
	m_document->SetDirty(&reg,m_whichLayer);

	delete m_original;
	delete m_perverted;
}

void BHandTool::DrawIcon(BView *drawInto, BRect location)
{
	drawInto->StrokeLine(location.LeftTop(),location.RightBottom());
	drawInto->StrokeLine(location.RightTop(),location.LeftBottom());
}
