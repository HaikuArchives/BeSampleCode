/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "List.h"
#include "Matrix.h"

class BBitmapView;
class BCheckBox;
class BBitmapDocument;
class BHandler;

struct LayerButton {
	BBitmapView *disp;
	BCheckBox *showing;
};

class BLayerView : public BMatrix
{
	protected:
	
		BList 				m_layers;
		BHandler *			m_handler;
		BBitmapDocument *	m_document;
		uint64				m_mask;
		BPicture *			m_cross;
		int32				m_trackingState;

	public:
	
						BLayerView(	BRect r, int32 xSize, int32 ySize,
									BBitmapDocument *doc, BHandler *handler);
						~BLayerView();

		virtual	void	FrameResized(float x, float y);
		virtual	void	AttachedToWindow();
		virtual	void	MessageReceived(BMessage *msg);
		virtual	void	DrawAfterChildren(BRect updateRect);

		virtual void	MouseUp(BPoint point);
		virtual void	MouseMoved(BPoint point, uint32 transit, const BMessage *message);

		void			AddLayer();
		void			DeleteLayer();

		void			ShowLayer(int32 layer);
		void			HideLayer(int32 layer);

		void			CellClicked(BPoint pt, int32 item, BRect itemRect);
		void			Select(int newSelection);
		void			SetHandler(BHandler *handler);
		void			LayoutViews();
};

