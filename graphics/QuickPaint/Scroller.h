/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <View.h>

class BScroller : public BView
{
	public:
						BScroller(const char *name, BView *scrolling);
						~BScroller();
				
				BRect	UpLeftArrow();
				BRect	DownRightArrow();
				BRect	ClientRect();

		virtual	void	Draw(BRect updateRect);
		virtual	void	FrameResized(float width, float height);
		virtual	void	Pulse();

		virtual	void	MessageReceived(BMessage *msg);
		
				void	ScrollUpLeft();
				void	ScrollDownRight();
				void	CheckArrows();

		virtual	void	MouseDown(BPoint pt);
		virtual void	MouseUp(BPoint point);
		virtual void	MouseMoved(BPoint point, uint32 transit, const BMessage *message);

	private:
	
				uint32	m_trackingState;
				int32	m_orientation;
				int32	m_stepSize;
				int32	m_clientExtent;
				BView *	m_client;
};
