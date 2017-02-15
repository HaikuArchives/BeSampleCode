/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _stroke_h
#define _stroke_h

#include <Archivable.h>
#include "minivec.h"

/////////////////////////////////////////////////////////////////////////////
// Class: PenStroke
// ----------------
// Represents a single pen stroke.
class PenStroke : public BArchivable
{
// static methods
public:
	static __declspec(dllexport) BArchivable*
		Instantiate(BMessage* archive);

// construction, destruction, operators
public:
	PenStroke(float fPenSize);
private:
	PenStroke(BMessage* archive);

// overrides
public:
	virtual status_t Archive(BMessage* archive, bool deep = true) const;

// accessors
public:
	BRect Bounds() const;
	void PushPoint(const BPoint& pt);
	int32 CountPoints() const;
	BPoint PointAt(int32 index) const;
	void PopPoint();
	
// operations
public:
	void DrawStroke(BView* target);
	void Fix();

// data members
private:
	float				m_fPenSize;
	MiniVec<BPoint>		m_pointList;
	BRect				m_rectBounds;

};

#endif /* _stroke_h */