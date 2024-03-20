/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "algobase.h"
#include <SupportDefs.h>
#include <Message.h>
#include <View.h>
#include "stroke.h"

/////////////////////////////////////////////////////////////////////////////
// PenStroke static methods

// PenStroke::Instantiate
// ----------------------
// Creates a new stroke from an archive.
//
// MFC NOTE: This is a wrapper around PenStroke(BMessage*) which is used
// by instantiate_object, the archiving mechanism.
BArchivable* PenStroke::Instantiate(BMessage* archive)
{
	if (validate_instantiation(archive, "PenStroke"))
		return new PenStroke(archive);
	else
		return 0;
}



/////////////////////////////////////////////////////////////////////////////
// PenStroke construction, destruction, operators

// PenStroke::PenStroke(float)
// ---------------------------
// Creates a stroke with the specified pen width.
PenStroke::PenStroke(float fPenSize)
	: m_fPenSize(fPenSize), m_rectBounds(0,0,0,0)
{ }

// PenStroke::PenStroke(BMessage*)
// -------------------------------
// Constructs a stroke from a message archive.
//
// MFC NOTE: This is how BArchivable objects are 'unserialized.' This is
// similar to Serialize where CArchive::IsStoring() == false. Compare this
// function to its counterpart, PenStroke::Archive.
PenStroke::PenStroke(BMessage* archive)
{
	m_rectBounds = archive->FindRect("bounds");
	m_fPenSize = archive->FindFloat("pen width");
	int32 i = 0;
	BPoint pt;
	while (archive->FindPoint("points", i++, &pt) == B_OK) {
		PushPoint(pt);
	}
}



/////////////////////////////////////////////////////////////////////////////
// PenStroke overrides

// PenStroke::Archive
// ------------------
// Saves the state of the stroke into the archive.
//
// MFC NOTE: This is how BArchivable objects are 'serialized.' This is
// similar to Serialize where CArchive::IsStoring() == true. Compare this
// function to its counterpart, PenStroke::PenStroke(BMessage*).
status_t PenStroke::Archive(BMessage* archive, bool /* deep */) const
{
	status_t res = BArchivable::Archive(archive);
	if (res != B_OK) {
		return res;
	}
	archive->AddRect("bounds", m_rectBounds);
	archive->AddFloat("pen width", m_fPenSize);
	int32 len = CountPoints();
	for (int32 i=0; i < len; i++) {
		BPoint pt = PointAt(i);
		archive->AddPoint("points", pt);
	}
	return B_OK;
}



/////////////////////////////////////////////////////////////////////////////
// PenStroke accessors

BRect PenStroke::Bounds() const
{
	return m_rectBounds;
}

void PenStroke::PushPoint(const BPoint& pt)
{
	m_pointList.Push(pt);
}

int32 PenStroke::CountPoints() const
{
	return m_pointList.CountItems();
}

BPoint PenStroke::PointAt(int32 index) const
{
	return m_pointList.ItemAt(index);
}

void PenStroke::PopPoint()
{
	m_pointList.Pop();
}



/////////////////////////////////////////////////////////////////////////////
// PenStroke operations

// PenStroke::DrawStroke
// ---------------------
// Renders the stroke into the view.
//
// MFC NOTE: There is no 'device context' in the BeOS API: the view
// handles all of the drawing commands. 
void PenStroke::DrawStroke(BView* view)
{
	int32 len = CountPoints();
	if (len < 2)
		return;
	
	// MFC NOTE: Note the absence of SelectObject. In the BeOS, the view
	// contains a stack frame of graphics parameters, such as pen size,
	// font, and colors, which get pushed onto and popped from the stack
	// during a normal Draw operation. This means that you can set the
	// graphics parameters to whatever you want, without worrying about
	// restoring the original values; the graphics state stack will be
	// popped once BView::Draw is finished.
	rgb_color color = view->HighColor();
	view->SetPenSize(m_fPenSize);
	if (m_fPenSize > 2) {
		view->SetLineMode(B_ROUND_CAP, B_ROUND_JOIN); // for smooth strokes
	} else {
		view->SetLineMode(B_SQUARE_CAP, B_ROUND_JOIN); // improve visibiltiy of thin strokes
	}
	
	// We're using line arrays for improved performance.
	view->BeginLineArray(len - 1);
	BPoint prevPt = PointAt(0);
	for (int32 i=1; i<len; i++) {			
		BPoint curPt = PointAt(i);
		view->AddLine(prevPt, curPt, color);
		prevPt = curPt;
	}
	view->EndLineArray();
}

// PenStroke::Fix
// --------------
// Fix the bounding box based on the stroke's data.
void PenStroke::Fix()
{
	int32 len = CountPoints();
	if (len == 0) {
		m_rectBounds.Set(0,0,0,0);
		return;
	}
	
	BPoint pt = PointAt(0);
	m_rectBounds.Set(pt.x, pt.y, pt.x, pt.y);

	for (int32 i=1; i < len; i++)
	{
		// If the point lies outside of the accumulated bounding
		// rectangle, then inflate the bounding rect to include it.
		// MFC NOTE: note that bottom > top in the default
		// BeOS view coordinates, since the y axis points downwards.
		pt = PointAt(i);
		m_rectBounds.left     = min_c(m_rectBounds.left, pt.x);
		m_rectBounds.right    = max_c(m_rectBounds.right, pt.x);
		m_rectBounds.top      = min_c(m_rectBounds.top, pt.y);
		m_rectBounds.bottom   = max_c(m_rectBounds.bottom, pt.y);
	}

	// Add the pen width to the bounding rectangle.  This is necessary
	// to account for the width of the stroke when invalidating
	// the screen.
	// MFC NOTE: BRect::InsetBy with negative values is equivalent
	// to CRect::InflateRect.
	m_rectBounds.InsetBy(-m_fPenSize, -m_fPenSize);
}

