/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Bitmap.h>
#include <Box.h>
#include <Control.h>

rgb_color color(uint8, uint8, uint8);

class TCrayon {
public:
				TCrayon(BControl* parent, BRect frame, rgb_color color);
				~TCrayon();

	void		DrawCrayon();

	rgb_color	Color() const;	
	void		SetColor(rgb_color);
	
	BRect		Frame() const { return fFrame; }
private:
	BControl*		fParent;
	BRect		fFrame;
	rgb_color	fColor;
	BBitmap*	fIcon;						
};

const int32 kMaxCrayonCount = 16;
class TCrayonColorPicker : public BControl {
public:
				TCrayonColorPicker(BPoint);
				~TCrayonColorPicker();
		
	void		Draw(BRect);
	void		MouseDown(BPoint where);
				
	void		GetPreferredSize(float*, float*) const;
	void		ResizeToPreferred();
	
	rgb_color	CurrentColor() const;
	void		SetCurrentColor(rgb_color);

	void		AddCrayon(rgb_color);	
private:
	TCrayon*	fCrayonList[kMaxCrayonCount];
	int32		fCrayonCount;
	
	TCrayon*	fCurrentColor;			
};

