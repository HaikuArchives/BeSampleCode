/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "PaintApp.h"
#include "Matrix.h"
#include "Region.h"
#include "Window.h"

#include <stdio.h>

BMatrix::BMatrix(
	BRect r, char *name,
	int32 matrixX, int32 matrixY, bool inset,
	int32 xSize, int32 ySize,
	uint32 follow, uint32 flags) 
	: BView(r,name,follow,flags|B_WILL_DRAW|
		(((xSize == DYNAMIC_SIZE) || (ySize == DYNAMIC_SIZE))?B_FULL_UPDATE_ON_RESIZE:0))
{
	m_selection = 0;
	m_slots = matrixX * matrixY;
	m_slotsX = matrixX;
	m_slotsY = matrixY;
	m_xSize = xSize;
	m_ySize = ySize;
	m_inset = inset;
	SetViewColor(B_TRANSPARENT_32_BIT);
};

BMatrix::~BMatrix()
{
};

void BMatrix::SendExtents()
{
	BMessage m(bmsgExtents);
	if (m_xSize != DYNAMIC_SIZE)
		m.AddInt32("x",m_xSize * m_slotsX);
	else
		m.AddInt32("x",Bounds().IntegerWidth());
	if (m_ySize != DYNAMIC_SIZE)
		m.AddInt32("y",m_ySize * m_slotsY);
	else
		m.AddInt32("y",Bounds().IntegerHeight());
	Window()->PostMessage(&m,Parent());
}

void BMatrix::AttachedToWindow()
{
	SendExtents();
}

void BMatrix::SetXSlots(int32 xSlots)
{
	m_slotsX = xSlots;
	m_slots = m_slotsX * m_slotsY;
	SendExtents();
}

void BMatrix::SetYSlots(int32 ySlots)
{
	m_slotsY = ySlots;
	m_slots = m_slotsX * m_slotsY;
	SendExtents();
}

BRect BMatrix::ItemRect(int item)
{
	BRect r,b = Bounds();
	b.OffsetTo(B_ORIGIN);
	float width=m_xSize,height=m_ySize;
	
	if (width == DYNAMIC_SIZE)
		width = (b.Width()-1) / m_slotsX;
	if (height == DYNAMIC_SIZE)
		height = (b.Height()-1) / m_slotsY;

	int32 add = (int32)m_inset;

	r.left = add+floor(width*(item%m_slotsX));
	r.right = add+floor(width*((item%m_slotsX)+1))-1;
	r.top = add+floor(height*(item/m_slotsX));
	r.bottom = add+floor(height*((item/m_slotsX)+1))-1;
	return r;
};

void BMatrix::Select(int newSelection)
{
	if (newSelection == m_selection) return;

	int32 oldSelection = m_selection;
	m_selection = newSelection;
	Window()->DisableUpdates();
	Invalidate(ItemRect(oldSelection));
	Invalidate(ItemRect(m_selection));
	Window()->EnableUpdates();
};

void BMatrix::DrawItem(int item, BRect itemRect)
{
};

void BMatrix::Draw(BRect updateRect)
{
	static rgb_color light = {240,240,240,255};
	static rgb_color mezzolight = {190,190,190,255};
	static rgb_color mezzodark = {160,160,160,255};
	static rgb_color dark = {130,130,130,255};
	rgb_color ul,lr;
	BRect r,b;

	BeginLineArray(8*m_slots);

	b = ItemRect(m_slots-1);
	b.top = b.left = 0;
	
	if (m_inset) {
		b.right++;
		b.bottom++;
		AddLine(b.LeftTop(),b.RightTop(),dark);
		AddLine(b.LeftTop(),b.LeftBottom(),dark);
		AddLine(b.LeftBottom(),b.RightBottom(),light);
		AddLine(b.RightBottom(),b.RightTop(),light);
	};
	
	for (int i=0;i<m_slots;i++) {
		r = ItemRect(i);
		if (m_selection == i) {
			ul = dark;
			lr = light;
			SetHighColor(mezzodark);
		} else {
			ul = light;
			lr = dark;
			SetHighColor(mezzolight);
		};
		AddLine(r.LeftTop(),r.RightTop(),ul);
		AddLine(r.LeftTop(),r.LeftBottom(),ul);
		AddLine(r.LeftBottom(),r.RightBottom(),lr);
		AddLine(r.RightBottom(),r.RightTop(),lr);
		r.InsetBy(1,1);
		FillRect(r);
		SetHighColor(0,0,0);
		r.InsetBy(2,2);
		DrawItem(i,r);
	};
	EndLineArray();
	
	BRegion reg;
	reg.Set(Bounds());
	reg.Exclude(b);
	SetHighColor(mezzolight);
	FillRegion(&reg);
};

void BMatrix::CellClicked(BPoint pt, int32 item, BRect itemRect)
{
	Select(item);
}

int32 BMatrix::ItemAt(BPoint pt)
{
	for (int i=0;i<m_slots;i++) {
		BRect r = ItemRect(i);
		if (r.Contains(pt)) return i;
	};
	return -1;
}

void BMatrix::MouseDown(BPoint point)
{
	int32 i;
	if ((i=ItemAt(point)) >= 0)
		CellClicked(point,i,ItemRect(i));
};
