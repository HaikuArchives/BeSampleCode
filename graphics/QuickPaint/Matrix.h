/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef Matrix_h
#define Matrix_h

#include <View.h>

#define DYNAMIC_SIZE -1

class BMatrix : public BView
{
	protected:

		int32			m_xSize,m_ySize;
		int32			m_slots,m_slotsX,m_slotsY;
		int32			m_selection;
		bool			m_inset;
		
	public:
						BMatrix(BRect r, char *name,
								int32 xMatrix, int32 yMatrix, bool inset=true,
								int32 xSize=DYNAMIC_SIZE, int32 ySize=DYNAMIC_SIZE,
								uint32 follow=B_FOLLOW_ALL, uint32 flags=B_WILL_DRAW);
						~BMatrix();

		inline	int32	Selection() { return m_selection; };
				
				BRect	ItemRect(int item);
				int32	ItemAt(BPoint pt);

				void	SendExtents();
				void	SetXSlots(int32 xSlots);
				void	SetYSlots(int32 ySlots);

		virtual	void	AttachedToWindow();
		virtual	void	Select(int newSelection);
		virtual	void	Draw(BRect updateRect);
		virtual	void	DrawItem(int item, BRect itemRect);
		virtual	void	MouseDown(BPoint p);
		virtual	void	CellClicked(BPoint pt, int32 item, BRect itemRect);
};

#endif
