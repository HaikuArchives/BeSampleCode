#ifndef _BITMAPVIEW_H_
#define _BITMAPVIEW_H_

#include <interface/View.h>
#include <interface/Bitmap.h>

class BitmapView : public BView {
public:
	BitmapView(BRect bounds,BBitmap* image = NULL);
	~BitmapView(void);
	
	virtual void Draw(BRect updateRect);
	void SetImage(BBitmap* image);
	
private:
	BBitmap* mImage;
};

#endif // #ifndef _BITMAPVIEW_H_