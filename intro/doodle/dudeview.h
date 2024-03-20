/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _dudeview_h
#define _dudeview_h

/////////////////////////////////////////////////////////////////////////////
// Class: DudeView
// ---------------
// A view which displays a DudeDoc's data.
//
// MFC NOTE: This class is directly analagous to CView.

#include <PrintJob.h>
#include <View.h>

class DudeDoc;

class DudeView : public BView
{
// construction, destruction, operators
public:
	DudeView(BRect frame, const char* name);
	virtual ~DudeView();
	
// overrides
public:
	virtual void Draw(BRect updateRect);
	virtual void MouseDown(BPoint pt);
	virtual void MouseMoved(BPoint pt, uint32 transit, const BMessage* drag);
	virtual void MouseUp(BPoint pt);
	virtual void MessageReceived(BMessage* message);
	virtual void AttachedToWindow();
	virtual void FrameResized(float width, float height);
	
// operations
public:
	status_t	Init(DudeDoc* pDoc);
	
// message handlers
private:
	void		OnUpdate(BMessage* message);
	status_t	OnPageSetup();
	void		OnPrint();
	
// implementation
private:
	bool		PrintPage(BPrintJob& printJob, int32 nPage);
	void		PrintTitlePage(BPrintJob& printJob, const char* title);
	void		PrintPageHeader(BPrintJob& printJob, BRect* printableRect,
					const char* text);
					
	void		SetScrollBars();

// data members
private:
	DudeDoc* 	m_pDocument;
	PenStroke*	m_pCurStroke;
	BPoint		m_ptPrev;
	bool		m_bTrackMouse;
};

#endif /* _dudeview_h */
