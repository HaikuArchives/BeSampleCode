/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "PaintApp.h"
#include "Toolbox.h"
#include "malloc.h"

rgb_color rgbBlack = {0,0,0,255};

#define bmsgToolInvocation	'tinv'

BToolbox::BToolbox(BRect r, int32 matrixX, int32 matrixY) 
	: BMatrix(r,"Toolbox",matrixX,matrixY,true,DYNAMIC_SIZE,DYNAMIC_SIZE,B_FOLLOW_ALL,0)
{
	m_tools = (BTool**)malloc(sizeof(BTool*) * m_slots);
	for (int i=0;i<m_slots;i++) m_tools[i] = NULL;

	m_selection = -1;
	Select(0);
};

BToolbox::~BToolbox()
{
	for (int i=0;i<m_slots;i++) 
		if (m_tools[i]) delete m_tools[i];
	free(m_tools);
};

status_t BToolbox::AddTool(BTool *tool)
{
	for (int i=0;i<m_slots;i++) {
		if (!m_tools[i]) {
			m_tools[i] = tool;
			Window()->Lock();
			Invalidate(ItemRect(i));
			Window()->Unlock();
			return B_OK;
		};
	};
	
	return B_ERROR;
};

void BToolbox::DrawItem(int i, BRect r)
{
	if (m_tools[i]) m_tools[i]->DrawIcon(this,r);
};

void BToolbox::Select(int newSelection)
{
	if (newSelection == m_selection) return;
	BMatrix::Select(newSelection);
};

void BToolbox::InvokeTool(BMessage *m)
{
	if (m_tools[m_selection]) m_tools[m_selection]->MessageReceived(m);
};

BToolboxWindow::BToolboxWindow(BRect r)
	: BWindow(r,"ToolBox",
		B_FLOATING_WINDOW_LOOK,B_FLOATING_APP_WINDOW_FEEL,
		B_NOT_RESIZABLE|B_WILL_ACCEPT_FIRST_CLICK|B_ASYNCHRONOUS_CONTROLS)
{
	r.OffsetTo(B_ORIGIN);
	BView *top = new BView(r,NULL,B_FOLLOW_ALL,B_WILL_DRAW);
	AddChild(top);
	top->SetViewColor(190,190,190);
	r.bottom = r.top+100;
	m_toolbox = new BToolbox(r,6,2);
	m_colorSelector = new BColorControl(BPoint(10,r.bottom+11),
		B_CELLS_32x8,6,"Color Selector",NULL,false);
	m_colorSelector->SetViewColor(190,190,190);
	m_colorSelector->SetResizingMode(B_FOLLOW_BOTTOM|B_FOLLOW_H_CENTER);
	r = m_colorSelector->Frame();
	r.left = r.top = 0;
	r.right += 10;
	r.bottom += 10;
	ResizeTo(r.Width(),r.Height());
	m_toolbox->ResizeTo(r.Width(),m_toolbox->Bounds().Height());
	top->AddChild(m_colorSelector);
	top->AddChild(m_toolbox);
};
