/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef BitmapEditor_h
#define BitmapEditor_h

#include "BitmapView.h"

class BBitmapDocument;
class BFilePanel;

class BBitmapEditor : public BBitmapView
{
	protected:

		uint32				m_trackingState;
		int32				m_currentLayer;
		BFilePanel *		m_filePanel;

	public:
							BBitmapEditor(BRect rect, ulong resizeMode,
								ulong flags, BBitmapDocument *bitmap);
							~BBitmapEditor();
		
		inline	int32		CurrentLayer() { return m_currentLayer; };
				void		SetCurrentLayer(int32 layerMask);
		
				void		RefReceived(entry_ref *ref);

		virtual	void		MessageReceived(BMessage *msg);
		virtual void		MouseDown(BPoint point);
		virtual void		MouseUp(BPoint point);
		virtual void		MouseMoved(BPoint point, uint32 transit, const BMessage *message);
};

#endif
