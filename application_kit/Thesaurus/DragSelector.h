/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

//
//	Drag selector shows an icon which can be dragged onto a view in another application
//	to return a messenger to it.
//

#ifndef DRAGGER_H
#define DRAGGER_H

#include <View.h>

class Bitmap;

class DragSelector : public BView {
public:

					DragSelector(BRect);
					~DragSelector();
	void 			Enable();
	void 			Disable();
	virtual void 	Draw(BRect);
	virtual void 	MouseDown(BPoint);
	virtual void 	MessageReceived(BMessage *msg);

private:
	
	BBitmap 	*fBitmap;
	bool 		fEnabled;
};

#endif