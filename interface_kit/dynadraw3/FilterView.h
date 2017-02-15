/* FilterView.h */
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#include <View.h>
#include <List.h>

const int32 MIN_SLEEP = 3000;
const int32 MAX_SLEEP = 30000;

class FilterView : public BView
{
 public:
 	/* overridden BView functions */
 	FilterView(BRect R);
 	void MouseDown(BPoint point);
	void Draw(BRect updateRect);
	void MessageReceived(BMessage* msg);
	void Quit();

	/* printing helper functions */
	inline void SetScaleToPage(bool flag);
	inline void GetExtremities(int& maxx, int& maxy);
	
	/* accessor functions */
	inline int32 Mass()  const;
	inline int32 Drag()  const;
	inline int32 Width() const;
	inline int32 Sleep() const;
	inline rgb_color Color() const;
	
 private:
 	/* variables for the filter */
 	float curmass, curdrag, width;
 	float velx, vely, vel;
 	float accx, accy, acc;
 	float angx, angy;
 	BPoint odel, m, cur, last;
 	bool fillmode, fixed_angle; 
 	int32 zzz;
 	
 	/* printing helper variables */
 	bool scaleToPage;
 	float bigX, bigY;
 	
 	/* the list of the pen strokes */
	BList polyList;
	 
	/* the real work is done here */	
 	void DrawSegment();
 	bool Apply(BPoint m, float curmass, float curdrag);
 	
	/* deletes polygon list, invalidates the view */
	void ClearScreen();

 	/* little helper functions */
 	inline float flerp(float f0, float f1, float p);
 	inline void setpos(BPoint point);
 	inline void toggle(bool& x); 
};


/* inline functions */
int32 FilterView::Mass()  const { return (int32)(curmass*100); }
int32 FilterView::Drag()  const { return (int32)(curdrag*100); }
int32 FilterView::Width() const { return (int32)(width*100); }
int32 FilterView::Sleep() const { return (MAX_SLEEP-zzz); }
rgb_color FilterView::Color() const { return HighColor(); }

void  FilterView::SetScaleToPage(bool flag) { scaleToPage = flag; }
void  FilterView::GetExtremities(int& maxx, int& maxy)
		{ maxx = (int)bigX; maxy = (int)bigY; }

void  FilterView::toggle(bool& x) 
		{ x = ((x == true) ? false : true); }

float FilterView::flerp(float f0, float f1, float p)
		{ return ((f0*(1.0-p))+(f1*p)); }
