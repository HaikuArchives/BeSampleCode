/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <stdio.h>
#include <Alert.h>
#include <Autolock.h>
#include <PrintJob.h>
#include <Region.h>
#include <ScrollBar.h>
#include <StopWatch.h>
#include <StringView.h>
#include <Message.h>
#include "constants.h"
#include "dudedoc.h"
#include "dudeview.h"
#include "dudewin.h"
#include "stroke.h"

/////////////////////////////////////////////////////////////////////////////
// DudeView construction, destruction, operators

DudeView::DudeView(BRect frame, const char* name)
	: BView(frame, name, B_FOLLOW_ALL, B_FRAME_EVENTS | B_WILL_DRAW),
	m_pDocument(0), m_pCurStroke(0), m_bTrackMouse(false)
{ }

DudeView::~DudeView()
{
	if (! (m_pDocument && m_pDocument->WriteLock()))
		return;

	m_pDocument->RemoveView(this);
	m_pDocument->WriteUnlock();
}

/////////////////////////////////////////////////////////////////////////////
// DudeView overrides

// DudeView::Draw
// --------------
// Draws the view.
void DudeView::Draw(BRect /* updateRect */)
{
	// Lock the document so that its data doesn't change
	if (! (m_pDocument && m_pDocument->ReadLock()))
		return;
	
	if (IsPrinting()) {
		// MFC NOTE: In Be, the default scaling ("mapping mode")
		// on any printing device is 1.0 units : 1/72 inch. We
		// want to simulate MM_LOENGLISH, that is, 1.0 units :
		// 1/100 inch, so we need to scale down the drawing
		// a bit.
		SetScale(.72);
	}

	// Intersect the clipping region with the document bounds,
	// to make sure nothing gets drawn outside the document bounds
	// (especially when printing).	
	BRegion clipRegion;
	GetClippingRegion(&clipRegion);
	BRegion docClipRegion;
	docClipRegion.Set(m_pDocument->Bounds());
	clipRegion.IntersectWith(&docClipRegion);
	ConstrainClippingRegion(&clipRegion);
	
	int32 len = m_pDocument->CountStrokes();
	for (int32 i=0; i<len; i++) {
		PenStroke* pStroke = m_pDocument->StrokeAt(i);
		// ignore stroke if it's not in clipping region
		//
		// MFC NOTE: we don't have to deal with 'device'
		// coordinates in BeOS; the graphics architecture
		// takes care of this for us. We also get subpixel
		// precision for free, so there's no need to adjust
		// the bounding rect.
		if (! clipRegion.Intersects(pStroke->Bounds()))
			continue;
		pStroke->DrawStroke(this);
	}
	m_pDocument->ReadUnlock();
}

// DudeView::MouseDown
// -------------------
// Responds to any mouse down event at the specified point.
void DudeView::MouseDown(BPoint point)
{
	if (! m_pDocument)
		return;

	// determine if primary (left) mouse button is pressed
	int32 val;
	Window()->CurrentMessage()->FindInt32("buttons", &val);
	uint32 buttons = val;
	if (! (buttons & B_PRIMARY_MOUSE_BUTTON))
		return;
		
	// track mouse and draw stroke
	//
	// MFC NOTE: SetMouseEventMask is similar to MFC's
	// SetCapture, in that it causes mouse events to be
	// sent to our window no matter where they occur
	// on the screen. It's more flexible than MFC
	// capture in that any number of views can capture
	// events simultaneously, not the exclusive capture
	// afforded to MFC views.
	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
	m_bTrackMouse = true;
	m_ptPrev = point;
	m_pCurStroke = m_pDocument->NewStroke();
	m_pCurStroke->PushPoint(point);
}

// DudeView::MouseMoved
// --------------------
// Responds to any mouse moved event at the specified point.
void DudeView::MouseMoved(BPoint pt, uint32 /* transit */,
	const BMessage* /* drag */)
{
	// MFC NOTE: There's no GetCapture equivalent,
	// because any number of views can 'capture'
	// the mouse/keyboard at one time. So, we keep
	// track of whether we're tracking the mouse
	// by using a simple bool.
	if (! m_bTrackMouse)
		return;
		
	if (pt != m_ptPrev) {
		// new point detected; add it to the stroke
		m_pCurStroke->PushPoint(pt);
		
		// draw the new line using view functions
		// MFC NOTE: compare these to CPen, CDC::SelectObject, and
		// CDC::LineTo routines.
		SetPenSize(m_pDocument->CurPenSize());
		if (m_pDocument->CurPenSize() > 2) {
			SetLineMode(B_ROUND_CAP, B_ROUND_JOIN); // for smooth pen strokes
		} else {
			SetLineMode(B_SQUARE_CAP, B_ROUND_JOIN); // improve visibility of thin strokes
		}
		StrokeLine(m_ptPrev, pt);			
		m_ptPrev = pt;
	}
}	
	
// DudeView::MouseUp
// -----------------
// Responds to any mouse up event at the specified point.
void DudeView::MouseUp(BPoint /* pt */)
{
	if (! m_bTrackMouse)
		return;

	// we're done creating the stroke
	//
	// MFC NOTE: SetMouseEventMask automatically
	// turns itself off when the mouse buttons
	// are released, so we don't have to call
	// anything like ReleaseCapture.
	// BView::SetEventMask provides a persistent
	// 'capture' which lasts until you turn it off.
	m_pCurStroke->Fix();
	m_pDocument->UpdateAllViews(m_pCurStroke, this);
	m_bTrackMouse = false;
}

// DudeView::MessageReceived
// -------------------------
// The message dispatch routine for the view.
//
// MFC NOTE: Analagous to MFC's message map (but without the macros to
// hide the details, or VC++ to automatically generate the maps).
void DudeView::MessageReceived(BMessage* message)
{
	switch (message->what) {
	case MSG_FILE_PAGE_SETUP:
		OnPageSetup();
		break;
	case MSG_FILE_PRINT:
		OnPrint();
		break;
	case MSG_DOC_CONTENTS_CHANGED:
		OnUpdate(message);
		break;
	default:
		BView::MessageReceived(message);
		break;
	}
}

// DudeView::AttachedToWindow
// --------------------------
// Invoked when this view and its children have been added to the window.
void DudeView::AttachedToWindow()
{
	SetScrollBars();
}

// DudeView::FrameResized
// ----------------------
// Invoked when the view has been resized.
void DudeView::FrameResized(float /* width */, float /* height */)
{
	SetScrollBars();
}



/////////////////////////////////////////////////////////////////////////////
// DudeView operations

// DudeView::Init
// --------------
// Initializes the view to point to the specified document.
status_t DudeView::Init(DudeDoc* pDoc)
{
	m_pDocument = pDoc;
	m_pDocument->AddView(this);
	return B_OK;
}



/////////////////////////////////////////////////////////////////////////////
// DudeView message handlers

// DudeView::OnUpdate
// ------------------
// Updates the view based on the message's contents.
void DudeView::OnUpdate(BMessage* message)
{
	BRect bounds;
	if (message->FindRect("bounds", &bounds) == B_OK) {
		Invalidate(bounds);
	} else {
		Invalidate();
	}
}

// DudeView::OnPageSetup
// ---------------------
// Runs the standard File>Page Setup dialog. Returns B_OK if successful.
//
// MFC NOTE: The MFC architecture handles this automatically.
status_t DudeView::OnPageSetup()
{
	// make sure we have exclusive access to the document
	if (! m_pDocument)
		return B_ERROR;

	// MFC NOTE: BPrintJob handles all of the commands and info associated
	// with a print job. It's a more comprehensive class than CPrintInfo,
	// because it controls the actual print operation as well.
	BPrintJob printJob(m_pDocument->Title());
	
	// MFC NOTE: The print settings are stored in a message, in an
	// undocumented format. BPrintJob is in charge of reading and
	// writing these settings, and we are in charge of storing them.
	BMessage* printSettings = m_pDocument->PrintSettings();
	if (printSettings) {
		// we transfer ownership of the print settings to the print job
		printJob.SetSettings(new BMessage(*printSettings));
	}
	
	// ConfigPage launches the File>Page Setup dialog.	
	status_t result = printJob.ConfigPage();
	if ((result == B_OK) && (m_pDocument->WriteLock())) {
		// Page Setup was successful; store the new settings.
		// Once we call printJob.Settings(), we own the pointer
		// that it returns.
		m_pDocument->SetPrintSettings(printJob.Settings());
		m_pDocument->SetModifiedFlag();
		m_pDocument->WriteUnlock();
	} else {
		result = B_ERROR;
	}
	
	return result;
}

// DudeView::OnPrint
// -----------------
// Handles the File>Print command.
//
// MFC NOTE: The MFC architecture handles this automatically,
// and only gives you hooks into the printing process. This
// approach is a more flexible one.
void DudeView::OnPrint()
{
	// make sure we have exclusive access to the document
	if (! m_pDocument)
		return;

	// create the object which will manage the print process
	BPrintJob printJob(m_pDocument->Title());
	
	// run File>Page Setup if the user hasn't already specified
	// page setup commands for this document
	BMessage* printSettings = m_pDocument->PrintSettings();
	if (! printSettings) {
		if (OnPageSetup() != B_OK) {
			// Page Setup cancelled
			return;
		} else {
			// Page Setup successful; reacquire the print
			// settings
			printSettings = m_pDocument->PrintSettings();
		}
	}
	
	// update the print job with the document's current
	// Page Setup settings.
	printJob.SetSettings(new BMessage(*printSettings));
	
	// run the standard File>Print dialog to determine
	// page range, # of copies, etc.
	if (printJob.ConfigJob() != B_OK) {
		return;
	}
	
	// determine page range
	int32 nFirst = printJob.FirstPage();
	int32 nLast = printJob.LastPage();
	int32 nTotal = 2; 	// 2 pages total;
						// page 1 -> Title page
						// page 2 -> Document
	
	if (nFirst < 1)
		nFirst = 1;
	if (nLast < 1)
		nLast = 1;
	if (nFirst >= nTotal)
		nFirst = nTotal;
	if (nLast >= nTotal)
		nLast = nTotal;
	
	// start the printing process
	bool bOk = true;
	printJob.BeginJob();

	for (int32 i = nFirst; i <= nLast; i++) {
		bOk = PrintPage(printJob, i);
		if (! bOk)
			break;
	}
	
	if (bOk) {
		// no errors; complete the printing process
		printJob.CommitJob();
	} else {
		// errors in printing; inform the user and bail
		BAlert* pAlert = new BAlert("print error",
			"There was an error printing the document.", "OK");
		pAlert->Go();
	}	
}



/////////////////////////////////////////////////////////////////////////////
// DudeView implementation

// DudeView::PrintPage
// -------------------
// Prints the specified page to the specified print job.
//
// MFC NOTE: similar to CView::OnPrint.
bool DudeView::PrintPage(BPrintJob& printJob, int32 nPage)
{
	BWindow* pWin = Window();
	if (! (pWin && m_pDocument && m_pDocument->ReadLock())) {
		return false;
	}
	
	const char* title = m_pDocument->Title();
	
	if (nPage == 1) {
		// page 1: title page
		PrintTitlePage(printJob, title);
	} else if (nPage == 2) {
		// page 2: document page
		BRect r = printJob.PrintableRect();	// relative to left top corner of paper
		r.OffsetTo(0,0);	// we draw relative to left top corner of printable area
		PrintPageHeader(printJob, &r, title);	// prints header and adjusts
												// printable area
		printJob.DrawView(this, m_pDocument->Bounds(), r.LeftTop());
	} else {
		m_pDocument->ReadUnlock();
		return false;
	}
	m_pDocument->ReadUnlock();
			
	printJob.SpoolPage();
	bool ok = printJob.CanContinue();
	return ok;
}

// DudeView::PrintTitlePage
// ------------------------
// Prints a page containing the specified title.
void DudeView::PrintTitlePage(BPrintJob& printJob, const char* title)
{
	BWindow* pWin = Window();
	BRect pageBounds = printJob.PrintableRect();
	
	// start 1 in. (72 points) down
	// MFC NOTE: measurements are given in Be's default typographical
	// coordinates (1 unit = 1 point = 1/72 inch).
	pageBounds.OffsetTo(0,72);
	
	// use a simple string view for drawing
	BStringView* pTitle = new BStringView(pageBounds, "title", title);

	// we want a font 3/4 inch (54 points) in size
	// MFC NOTE: Compare to CDC::SelectObject(CFont*). We don't use
	// indirect font selection; in this case, I use the stock plain
	// font and scale it. Also note that we don't have to reset the
	// font when we're done; the graphics architecture takes care
	// of that for us.
	BFont font(be_plain_font);
	font.SetSize(54);
	pTitle->SetFont(&font);
	
	// resize the view's bounds to encompass the text height
	// MFC NOTE: these measurements are analagous to tmHeight.
	font_height fontHeight;
	font.GetHeight(&fontHeight);
	pTitle->ResizeTo(pageBounds.Width(), fontHeight.ascent + fontHeight.descent);
		
	// view needs to be attached to a window to be drawn
	pTitle->Hide();	// so it won't be seen on the screen
	pWin->AddChild(pTitle);
	
	// center view in drawing space
	pTitle->SetAlignment(B_ALIGN_CENTER);
	
	// draw entire view 1 in. down
	printJob.DrawView(pTitle, pTitle->Bounds(), pageBounds.LeftTop());
	
	// cleanup
	pWin->RemoveChild(pTitle);
	delete pTitle;
}

// Class: HeaderView
// -----------------
// A simple view which draws the header.
//
// MFC NOTE: In MFC, you can draw the header in CView::OnPrint simply by
// drawing to the CDC before executing OnDraw. In Be, however, we need to
// have a view for rendering, so we derive a view which accomplishes this.
class HeaderView : public BView
{
public:
	HeaderView(float width, const char* text);
	virtual void Draw(BRect updateRect);
private:
	float m_fTextHeight;
	float m_fAboveText;
	float m_fBelowText;
	float m_fBelowLine;
	const char* m_text;
};

// DudeView::PrintPageHeader
// -------------------------
// Prints a header at the top of printableRect which contains the specified
// text. The space occupied by the header is subtracted from printableRect.
void DudeView::PrintPageHeader(BPrintJob& printJob, BRect* printableRect,
	const char* text)
{
	BWindow* pWin = Window();

	// create a view which draws the header	
	HeaderView* pHeaderView = new HeaderView(printableRect->Width(), text);
	
	// view needs to be attached to a window to be drawn
	pHeaderView->Hide();	// so the view won't be seen on screen
	pWin->AddChild(pHeaderView);
	
	// now draw the entire view at the left top corner of the printable rect
	printJob.DrawView(pHeaderView, pHeaderView->Bounds(), printableRect->LeftTop());
	
	// subtract the header's height from the printable area
	printableRect->top += pHeaderView->Bounds().Height() + 1;
	
	// clean up
	pWin->RemoveChild(pHeaderView);
	delete pHeaderView;
}

// HeaderView::HeaderView
// ----------------------
// Creates a header view with the specified width, which displays the specified
// text.
HeaderView::HeaderView(float width, const char* text)
	: BView(BRect(0,0,0,0), "header", B_FOLLOW_NONE, B_WILL_DRAW), m_text(text)
{
	// calculate the view's layout dimensions
	//
	// MFC NOTE: all of these measurements are given in the BeOS's default
	// typographical coordinates (1 unit = 1 point = 1/72 inch). We could
	// measure this in LOENGLISH units and scale the view in HeaderView::Draw,
	// similarly to DudeView::Draw, if we wanted.
	
	// .25 in. from top of view to the text
	m_fAboveText = 18;
	// .35 inches from bottom of text to ruler
	m_fBelowText = 26;
	// .25 inches from ruler to bottom of view
	m_fBelowLine = 18;
	// total height of text (also in points)
	font_height height;
	GetFontHeight(&height);
	m_fTextHeight = height.ascent + height.descent + height.leading;
	
	// resize view to the correct height and specified width
	float totHeight = m_fAboveText + m_fTextHeight + m_fBelowText + m_fBelowLine;		
	ResizeTo(width, totHeight);
}

// HeaderView::Draw
// ----------------
// Draws the header view.
void HeaderView::Draw(BRect /* updateRect */)
{
	// draw string m_fAboveText units down
	float y = m_fAboveText;
	DrawString(m_text, BPoint(0, y));
	// draw ruler m_fBelowText units below text
	y += m_fTextHeight + m_fBelowText;
	StrokeLine(BPoint(0, y), BPoint(Bounds().Width(), y));
}

// DudeView::SetScrollBars
// -----------------------
// Update the scroll bars associated with the view, based on the
// document size.
//
// MFC NOTE: CView::SetScrollSizes does this automatically.
void DudeView::SetScrollBars()
{
	if (! m_pDocument)
		return;
		
	BRect dataBounds = m_pDocument->Bounds();
	BRect viewBounds = Bounds();
	float scrollSize;
	// horizontal scroll bar
	BScrollBar* scroll = ScrollBar(B_HORIZONTAL);
	if (scroll) {
		scrollSize = dataBounds.Width() - viewBounds.Width();
		if (scrollSize > 0)
			// data bigger than view; set range appropriately
			scroll->SetRange(dataBounds.left, dataBounds.right - viewBounds.Width());
		else
			// view bigger than data; set range to 0
			scroll->SetRange(dataBounds.left, dataBounds.left);
		scroll->SetSteps(10, 100);
	}
	// vertical scroll bar
	scroll = ScrollBar(B_VERTICAL);
	if (scroll) {
		scrollSize = dataBounds.Height() - viewBounds.Height();
		if (scrollSize > 0)
			// data bigger than view; set range appropriately
			scroll->SetRange(dataBounds.top, dataBounds.bottom - viewBounds.Height());
		else
			// view bigger than data; set range to 0
			scroll->SetRange(dataBounds.top, dataBounds.top);
		scroll->SetSteps(10, 100);
	}
}
