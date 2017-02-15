/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <stdio.h>
#include "Tool.h"
#include "BitmapDocument.h"
#include "BitmapEditor.h"
#include "PaintApp.h"

/***********************************************/

BTool::BTool()
{
	m_trackingState = 0;
	m_eventMask = B_POINTER_EVENTS;
	m_eventOptions = B_LOCK_WINDOW_FOCUS;
	m_fullPixels = false;
};

void BTool::MessageReceived(BMessage *msg)
{
	rgb_color *cp;
	int32 i;
	
	switch (msg->what) {
		case bmsgToolInvocation:
		{
			/*
		
				"region"	: the area in which the tool is allowed to
							  have an effect (i.e. the bounds of the window)
				"bitmap"	: the bitmap on which the tool is acting
				"scale"		: the scale of the bitmap as it is displayed
							  (scale * bitmap unit = screen unit)
				"ulhc"		: "upper left hand corner" of the bitmap in the
							  client view's coordinate system
				"client"	: the BView requesting use of the tool
				"point"		: the location at which the tool was invoked
				"fore"		: current foreground color
				"back"		: current background color
				"button"	: the mouse button the tool was invoked with
		
			*/
			
			m_validRect = msg->FindRect("region");
			msg->FindPointer("bitmap",(void**)&m_document);
			msg->FindFloat("scale",&m_curScale);
			msg->FindPoint("ulhc",&m_curULHC);
			msg->FindPointer("client",(void**)&m_client);
			msg->FindPoint("point",&m_invokePoint);
			msg->FindData("fore",B_RGB_COLOR_TYPE,0,(const void**)&cp,&i); m_foreColor=*cp;
			msg->FindData("back",B_RGB_COLOR_TYPE,0,(const void**)&cp,&i); m_backColor=*cp;
			msg->FindInt32("button",(int32*)&m_invokedWith);
			msg->FindInt32("layer",(int32*)&m_whichLayer);
		
			m_document->Lock();
				m_document->View(m_whichLayer)->SetHighColor(m_foreColor);
				m_document->View(m_whichLayer)->SetLowColor(m_backColor);
			m_document->Unlock();
		
			BWindow *win = m_client->Window();
			win->Lock();
				m_client->SetMouseEventMask(m_eventMask,m_eventOptions);
			win->Unlock();
			m_invokePoint = PointOnBitmap(m_invokePoint);
			m_oldPoint = m_invokePoint;

			StartUsing();
			m_curOnBitmap = m_invokePoint;
			m_oldPoint = m_curOnBitmap;
			m_trackingState = 1;
			break;
		}
		case B_MOUSE_MOVED:
		{
			if (m_trackingState) {
				m_curOnBitmap = PointOnBitmap(msg->FindPoint("be:view_where"));
				if ((m_curOnBitmap.x != m_oldPoint.x) ||
					(m_curOnBitmap.y != m_oldPoint.y) ) {
					PointerMoved(m_curOnBitmap);
					m_oldPoint = m_curOnBitmap;
				};
			};
			break;
		}
		case B_MOUSE_UP:
		{
			if (m_trackingState) {
				StopUsing();
				m_trackingState = 0;
			};
			break;
		};
		default: break;
	};
};

BPoint BTool::PointOnBitmap(BPoint pointOnClient)
{
	BPoint p;
	p.x = (pointOnClient.x+m_curULHC.x)/m_curScale;
	p.y = (pointOnClient.y+m_curULHC.y)/m_curScale;
	if (m_fullPixels) {
		p.x = floor(p.x);
		p.y = floor(p.y);
	};
	return p;
};

