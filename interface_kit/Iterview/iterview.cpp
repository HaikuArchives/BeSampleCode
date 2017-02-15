/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Application.h>
#include <Window.h>
#include <View.h>
#include <Shape.h>
#include <Picture.h>
#include <String.h>
#include <stdlib.h>
#include <TranslatorRoster.h>
#include <TranslationUtils.h>
#include <Bitmap.h>

#define TEXT "BeOS"

/* glyphPt is a little convience class for holding a pointer */
/* to a control point and the offset of that point */

class glyphPt
{
 public:
	glyphPt(BPoint *p, BPoint o) { pt = p; offset = o;};
	BPoint *pt;
	BPoint offset;
};


class IterView : public BView, public BShapeIterator
{
 public:
					IterView(BRect R);
					~IterView();
		
	void			InitializeShapes();	

protected:

	void			MouseUp(BPoint pt);
	void 			MouseMoved(BPoint pt, uint32 code, const BMessage *msg);
	void			MouseDown(BPoint pt);
	void			Draw(BRect R);

	status_t		IterateMoveTo(BPoint *point);
	status_t		IterateLineTo(int32 lineCount, BPoint *linePts);
	status_t		IterateBezierTo(int32 bezierCount, BPoint *bezierPts);
	status_t		IterateClose();

 private:

	BList 			*shapePts;

	escapement_delta delta;
	float			fontSize;
	
	BShape			**shapes;
	BPoint			*esc;
	BPoint			offset;

	BString			text;
	int				textlen;
	BPoint			initialPoint;

	glyphPt			*dragPoint;
	
	BPoint			currentPoint;
	int				curShape;
	
	bool			firstPass;	
	bool			isTracking;
	
};


IterView::IterView(BRect R)
	: BView(R, "iterview", B_FOLLOW_ALL, B_WILL_DRAW),
	  BShapeIterator()
{
	text = TEXT;
	textlen = text.Length();
	
	/* create an array of BLists to hold the control points */
	/* of each glyph */
	curShape = 0;
	shapePts = new BList[textlen];
	
	fontSize = 350;	 
	initialPoint.Set(10,350);

	firstPass = true;
	isTracking = false;

	/* escapements for the text string */	
	esc = new BPoint[textlen*2];

	/* the BShapes for the glyphs */
	shapes = (BShape**)malloc(sizeof(BShape*)*textlen);
	for (int32 i=0;i<textlen;i++)
	{
		shapes[i] = new BShape();
	}

	/* get the shapes and escapements for the glyphs */	
	InitializeShapes();

	SetViewColor(255,255,255);
}

IterView::~IterView()
{
	int c;

	for (int32 i=0;i<textlen;i++)
	{
		delete shapes[i];	
		c = shapePts[i].CountItems();
		for(int j=0; j < c; j++)
		{
			shapePts[i].RemoveItem(0L);
		}
	}
	delete []shapePts;
	delete []esc;
	free(shapes);
}

void
IterView::InitializeShapes()
{
	BFont font;
	GetFont(&font);
	font.SetSize(fontSize);
	
	delta.nonspace = 0.0;
	delta.space = 0;

	font.GetGlyphShapes(text.String(), textlen, shapes);
	font.GetEscapements(text.String(), textlen, &delta, esc, esc+textlen);
}


void
IterView::MouseDown(BPoint pt)
{
	uint32 buttons=1;

	BPoint diff;
	glyphPt *g;
	bool foundPt = false;
	
	for(int j=0; j < textlen; j++)
	{
		/* get the bounding box for the current BShape */
		BRect Tmp = shapes[j]->Bounds();
		/* enlarge it a little so we can click on the edges */
		Tmp.InsetBy(-3,-3);
		/* take the offset of one of the control points of the BShape */
		Tmp.OffsetBy(((glyphPt*)shapePts[j].ItemAt(0))->offset);
		
		if(Tmp.Contains(pt))
		{
			for(int i=0; i < shapePts[j].CountItems(); i++)
			{
				g = (glyphPt*)shapePts[j].ItemAt(i);
				
				/* see if this point is "close to" the mouse down pt */
				diff = *(g->pt) + g->offset - pt;		
				if(diff.x > -3 && diff.x < 3 && diff.y > -3 && diff.y < 3)
				{
					foundPt = true;
					break;
				}
			}
		}
		
		if(foundPt) break;
	}
	
	if(!foundPt) return;
		
	dragPoint = g;
	isTracking = true;	// we're tracking the mouse
	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS | B_NO_POINTER_HISTORY);
}


void
IterView::MouseMoved(BPoint pt, uint32 code, const BMessage *msg)
{
	if(isTracking == false) return;
	
	*(dragPoint->pt) = (pt - dragPoint->offset);
	Invalidate();	
}


void
IterView::MouseUp(BPoint pt)
{
	isTracking = false;
}


void
IterView::Draw(BRect R)
{
	BPoint where(initialPoint);
	
	curShape = 0;
	for (int32 i=0; i<textlen; i++)
	{
		offset.Set(floor(where.x+esc[i+textlen].x+0.5),
				  floor(where.y+esc[i+textlen].y+0.5)-1.0);

		/* stroke the glyph */
		MovePenTo(offset);
		SetHighColor(0,0,0);
		SetPenSize(2);
		StrokeShape(shapes[i]);
		SetPenSize(1);

		/* draw the control points */
		Iterate(shapes[i]);
		curShape++;

		where.x += esc[i].x * fontSize;
		where.y += esc[i].y * fontSize;
	};

	if(firstPass) firstPass = false;

}

status_t
IterView::IterateMoveTo(BPoint *point)
{
	SetHighColor(0,255,0);
	FillEllipse(*point+offset, 2, 2);
	if(firstPass)
	{
		shapePts[curShape].AddItem(new glyphPt(point, offset));
	}
	currentPoint = *point+offset;
	return B_OK;
}

status_t
IterView::IterateLineTo(int32 lineCount, BPoint *linePts)
{
	SetHighColor(255,0,0);
	for(int i=0; i < lineCount; i++, linePts++)
	{
		FillEllipse(*linePts+offset, 2, 2);
		if(firstPass)
		{
			shapePts[curShape].AddItem(new glyphPt(linePts, offset));
		}
	}

	currentPoint = *(linePts-1)+offset;
	return B_OK;
}

status_t
IterView::IterateBezierTo(int32 bezierCount, BPoint *bezierPts)
{
	BPoint tmp(2.5,2.5);
	
	for(int i=0; i < bezierCount; i++, bezierPts = bezierPts+3)
	{

		SetHighColor(140,140,140);
		
		/* draw the handles as boxes */
		StrokeRect(BRect((*bezierPts + offset - tmp), (*bezierPts+offset+tmp)));
		StrokeRect(BRect((*(bezierPts+1) + offset - tmp), (*(bezierPts+1)+offset+tmp)));

		/* draw lines from the handles to the endpoints */
		StrokeLine(currentPoint,*bezierPts+offset);
		StrokeLine(*(bezierPts+1) + offset,*(bezierPts+2)+offset);

		/* draw the endpoint */
		SetHighColor(0,0,255);
		FillEllipse(*(bezierPts+2) + offset, 2, 2);
		
		if(firstPass)
		{
			shapePts[curShape].AddItem(new glyphPt(bezierPts, offset));
			shapePts[curShape].AddItem(new glyphPt(bezierPts+1, offset));
			shapePts[curShape].AddItem(new glyphPt(bezierPts+2, offset));
		}

		currentPoint = *(bezierPts+2) + offset;				
	}
	return B_OK;
}

status_t
IterView::IterateClose()
{
	return B_OK;
}

/***********************************************************************/

class TWin : public BWindow
{
 public:
				TWin();
				~TWin();
	bool		QuitRequested();
		
 private:
	IterView *view;
};

class TApp : public BApplication
{
 public:
				TApp();
				~TApp();
 private:
	TWin 		*win;
};


TApp::TApp()
	: BApplication("application/x-vnd.Be-clip")
{
	win = new TWin();
	Run();
}

TApp::~TApp()
{
	// empty
}

TWin::TWin()
	: BWindow(BRect(10,50,950,500), "BShapeIterator", B_TITLED_WINDOW, 0)
{
	view = new IterView(Bounds());
	AddChild(view);
	Show();
}

TWin::~TWin()
{
	// empty
}

bool
TWin::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

int
main()
{
	new TApp();
}
