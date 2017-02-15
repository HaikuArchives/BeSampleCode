/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#if !defined(DragView_h)
#define DragView_h

#include <View.h>
#include <TranslationDefs.h>

class BBitmap;
class BMenuField;

class DragView : public BView {
	friend class DragWindow;
public:
		DragView(
				const BRect & area);
virtual	~DragView();

virtual	void Draw(
				BRect area);
virtual	void MouseDown(
				BPoint where);

		enum {
			SET_DRAG_MODE = 'dv00'
		};
private:

		BBitmap * m_bitmap;				//	bitmap to drag
		BView * m_bitmapView;			//	for drawing into bitmap
		drawing_mode m_mode;			//	used for dragging
		translator_id m_translator;		//	the translator to use
		uint32 m_type_code;				//	the type to translate to
		char m_the_type[256];			//	the MIME type of that type
};

#endif	//	DragView_h
