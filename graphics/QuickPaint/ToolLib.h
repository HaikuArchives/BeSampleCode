/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef ToolLib_h
#define ToolLib_h

#include "Tool.h"

class BHandTool : public BTool
{

				BBitmap *			m_original;
				BBitmap *			m_perverted;
				BView *				m_drawer;
				BRect				m_limits;

	public:

									BHandTool();

		virtual	void				StartUsing();
		virtual	void				PointerMoved(BPoint newPoint);
		virtual	void				StopUsing();

		virtual	void				DrawIcon(BView *drawInto, BRect location);
};

class BPenTool : public BTool
{
	protected:

				BBitmap *			m_brush;
				BPoint 				m_hotspot;

	public:

									BPenTool();

		virtual	void				GenerateBrush();
		virtual void				SetDrawingOp();

		virtual	void				StartUsing();
		virtual	void				PointerMoved(BPoint newPoint);
		virtual	void				StopUsing();
		virtual	void				DrawIcon(BView *drawInto, BRect location);
};

class BAlphaTool : public BPenTool
{
	public:
		virtual	void				GenerateBrush();
		virtual void				SetDrawingOp();
		virtual	void				DrawIcon(BView *drawInto, BRect location);
};

class BRectBasedTool : public BTool
{
	private:

				BRect				m_theRect;

	protected:
	
				bool				m_fill;
				float				m_fudge;
			
	public:
									BRectBasedTool(bool fill);
		virtual	void				StartUsing();
		virtual	void				PointerMoved(BPoint newPoint);
		virtual	void				StopUsing();
		virtual	void				DrawIcon(BView *drawInto, BRect location);

		virtual	void				DrawPrimitive(BView *drawInto, BRect r)=0;
		virtual	void				InvalPrimitive(BRegion *inval, BRect r)=0;
		virtual	void				GetDeltaInval(BRegion *inval, BRect oldRect, BRect newRect)=0;
};

class BRectTool : public BRectBasedTool
{
	public:
									BRectTool(bool fill=false)
										: BRectBasedTool(fill) {};

		virtual	void				DrawPrimitive(BView *drawInto, BRect r);
		virtual	void				InvalPrimitive(BRegion *inval, BRect r);
		virtual	void				GetDeltaInval(BRegion *inval, BRect oldRect, BRect newRect);
};

class BEllipseTool : public BRectBasedTool
{
	public:
									BEllipseTool(bool fill=false)
										: BRectBasedTool(fill) {};

		virtual	void				DrawPrimitive(BView *drawInto, BRect r);
		virtual	void				InvalPrimitive(BRegion *inval, BRect r);
		virtual	void				GetDeltaInval(BRegion *inval, BRect oldRect, BRect newRect);
};

#endif
