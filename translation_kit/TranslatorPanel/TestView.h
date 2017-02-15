#ifndef TESTVIEW_H
#define TESTVIEW_H

#include <interface/View.h>
#include <interface/Bitmap.h>
#include <app/MessageRunner.h>
#include "addons/ImageFilter.h"

class TestView : public BView {
	public:
		TestView(BRect rect, const char *name);
		~TestView();
		void Draw(BRect rect);
		void FrameResized(float width, float height);
		void MouseMoved(BPoint point, uint32 state, const BMessage *message);
		void MouseDown(BPoint point);
		void MouseUp(BPoint point);
		void MessageReceived(BMessage *message);
		
		void SetImage(BMessage *message);
		void SaveImage(BMessage *message);
		bool HasImage();
		void AddFilter(ImageFilter *filter);
		void FilterImage(BMessage *message);

	private:
		void DragToTracker(BPoint point);
		void GetTrackerResponse(BMessage *message);
	
		BBitmap *bitmap;
		ImageFilter *filters;
		const BMessage *message_from_tracker;
		bool drag_from_tracker, drag_to_tracker;
		BMessageRunner *messagerunner;
};

#endif
