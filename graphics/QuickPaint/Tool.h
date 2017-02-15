/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef Tool_h
#define Tool_h

#include <AppKit.h>
#include <InterfaceKit.h>

class BBitmapDocument;
class BBitmapEditor;

class BTool : public BHandler
{

	protected:

				uint32				m_trackingState;
				int32				m_whichLayer;
				uint32				m_eventMask,m_eventOptions;
				float				m_curScale;
				BPoint				m_curULHC;
				rgb_color			m_foreColor,m_backColor;
				uint32				m_invokedWith;
				BPoint				m_invokePoint,m_oldPoint,m_curOnBitmap;
				BRect				m_validRect;
				BBitmapEditor *		m_client;
				BBitmapDocument *	m_document;
				bool				m_fullPixels;

		virtual	void				StartUsing() {};
		virtual	void				PointerMoved(BPoint /* newPoint */) {};
		virtual	void				StopUsing() {};
			
	public:

									BTool();
				BPoint				PointOnBitmap(BPoint pointOnClient);
		virtual	void				MessageReceived(BMessage *msg);
		virtual	void				DrawIcon(BView * /* drawInto */, BRect /* location */) {};
};

#endif
