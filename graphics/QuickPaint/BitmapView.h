/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef BitmapView_h
#define BitmapView_h

#define bmsgZoomIn		'zmin'
#define bmsgZoomOut		'zmot'
#define bmsgSetScale	'zset'

class BBitmapDocument;

class BBitmapView : public BView
{
	protected:

		BBitmapDocument *	m_bitmap;
		float				m_scale;
		uint64				m_layerMask;
		uint32				m_options;

	public:
							BBitmapView(BRect rect, ulong resizeMode,
								ulong flags, BBitmapDocument *bitmap);
							~BBitmapView();
		
	BBitmapDocument *		Document() { return m_bitmap; };
		
				void		SetScale(float scale);
				void		SetLayerMask(uint64 layerMask, int32 force=0);
				void		GetMaxSize(float *width, float *height);
				float		Scale();
				void		SetOptions(uint32 options);
		
		virtual	void		Draw(BRect updateRect);
		virtual	void		AttachedToWindow();
		virtual	void		MessageReceived(BMessage *msg);
		virtual	void		FrameResized(float width, float height);
				void		FixupScrollbars();
};

#endif
