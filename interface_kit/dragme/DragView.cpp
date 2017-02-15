/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Alert.h>
#include <Bitmap.h>
#include <Mime.h>
#include <TranslatorRoster.h>

#include <string.h>

#include "DragView.h"

#define BM_WID 95
#define BM_HEI 31


static void
make_image(
	BView * view)
{
	BRect r(view->Bounds());
	view->SetFont(be_bold_font);
	view->SetLowColor(255, 255, 255, 0);	//	transparent
	view->SetDrawingMode(B_OP_COPY);
	view->FillRect(r, B_SOLID_LOW);

	//	the colored squares are translucent when dragged
	BRect r2(r);
	r2.right = (r2.right+1)/3-1;
	r2.bottom = (r2.bottom+1)/2-1;
	view->SetHighColor(255,0,0, 128);
	view->FillRect(r2);

	r2.OffsetBy((r2.Width()+1)*2, 0);
	view->SetHighColor(0,255,0, 128);
	view->FillRect(r2);

	r2.OffsetBy(0,r2.Height()+1);
	view->SetHighColor(0,0,255, 128);
	view->FillRect(r2);

	r2.OffsetBy(-(r2.Width()+1)*2, 0);
	view->SetHighColor(255,0,255, 128);
	view->FillRect(r2);

	view->SetHighColor(64, 64, 64);
	view->SetFontSize(20);
	view->SetDrawingMode(B_OP_OVER);
	view->DrawString("Drag Me", BPoint((int)((BM_WID-view->StringWidth("Drag Me"))/2),24));
}

DragView::DragView(
	const BRect & area) :
	BView(area, "DragView", B_FOLLOW_ALL, B_WILL_DRAW)
{
	BRect r(0,0,BM_WID,BM_HEI);
	m_bitmap = new BBitmap(r, B_RGB32, true);
	m_bitmapView = new BView(r, "_bmv", B_FOLLOW_NONE, B_WILL_DRAW);
	m_bitmap->Lock();
	m_bitmap->AddChild(m_bitmapView);
	make_image(m_bitmapView);
	m_bitmapView->Sync();
	m_bitmap->Unlock();
	m_mode = B_OP_ALPHA;	//	mode to drag bitmap with

	//	Find the translator to use for us
	translator_id * all_translators = NULL;
	int32 count = 0;
	status_t err = BTranslatorRoster::Default()->GetAllTranslators(&all_translators, &count);
	if (err >= B_OK) {
		err = B_ERROR;
		//	look through all available translators
		for (int ix=0; ix<count; ix++) {
			const translation_format * in_formats;
			int32 fmt_count;
			if (B_OK <= BTranslatorRoster::Default()->GetInputFormats(all_translators[ix],
					&in_formats, &fmt_count)) {
				//	look for translators that accept BBitmaps as input
				for (int iy=0; iy<fmt_count; iy++) {
					if (in_formats[iy].type == B_TRANSLATOR_BITMAP) {
						goto do_this_format;
					}
				}
			}
			continue;
do_this_format:
			const translation_format * out_formats;
			if (B_OK <= BTranslatorRoster::Default()->GetOutputFormats(all_translators[ix],
					&out_formats, &fmt_count)) {
				//	look through the output formats
				for (int iy=0; iy<fmt_count; iy++) {
					//	and take the first output format that isn't BBitmap
					if (out_formats[iy].type != B_TRANSLATOR_BITMAP) {
						m_translator = all_translators[ix];
						m_type_code = out_formats[iy].type;
						strcpy(m_the_type, out_formats[iy].MIME);
						err = B_OK;
						break;
					}
				}
			}
		}
	}
	delete[] all_translators;
	if (err < B_OK) {
		(new BAlert("", "There are no graphics-export translators installed.", "Stop"))->Go();
		Window()->PostMessage(B_QUIT_REQUESTED);
	}
}

DragView::~DragView()
{
	delete m_bitmap;
}

void
DragView::Draw(
	BRect area)
{
	SetLowColor(216, 216, 216);
	SetHighColor(0,0,0);
	SetDrawingMode(B_OP_COPY);
	//	figure out where bitmap goes
	BRect r(m_bitmap->Bounds());
	int zz = (int)(Bounds().right-r.right)/2;
	r.OffsetBy(zz, zz);
	DrawBitmapAsync(m_bitmap, r.LeftTop());
	//	Draw gray area outside of frame
	BRect r2 = Bounds();
	r2.bottom = r.top-1;
	FillRect(r2, B_SOLID_LOW);
	r2.top = r.bottom+1;
	r2.bottom = Bounds().bottom;
	FillRect(r2, B_SOLID_LOW);
	r2.bottom = r.bottom;
	r2.top = r.top;
	r2.right = r.left-1;
	FillRect(r2, B_SOLID_LOW);
	r2.left = r.right+1;
	r2.right = Bounds().right;
	FillRect(r2, B_SOLID_LOW);
	//	Draw frame
	r2 = r;
	r2.InsetBy(-1, -1);
	StrokeRect(r2);
}

void
DragView::MouseDown(
	BPoint where)
{
	BRect r(m_bitmap->Bounds());
	int zz = (int)(Bounds().right-r.right)/2;
	r.OffsetBy(zz, zz);
	if (r.Contains(where)) {
		//	wait for a drag to start
		while (true) {
			BPoint w2;
			uint32 mods;
			GetMouse(&w2, &mods);
			if (!mods) {
				//	releasing the buttons without moving means no drag
				goto punt;
			}
			if (w2 != where) {
				//	moving the mouse starts a drag
				break;
			}
			snooze(40000);
		}
		BMessage msg(B_SIMPLE_DATA);
		//	add the types
		msg.AddString("be:types", B_FILE_MIME_TYPE);
		msg.AddString("be:types", m_the_type);
		msg.AddString("be:filetypes", m_the_type);
		//	add the actions
		msg.AddInt32("be:actions", B_COPY_TARGET);
		msg.AddInt32("be:actions", B_TRASH_TARGET);
		msg.AddString("be:clip_name", "DragMe Bitmap");
		//	create new bitmap to drag around
		BBitmap * bmp = new BBitmap(m_bitmap->Bounds(), m_bitmap->ColorSpace(), false);
		memcpy(bmp->Bits(), m_bitmap->Bits(), m_bitmap->BitsLength());
		//	start the drag (the rest will be handled by MessageReceived())
		BPoint pt(where.x-r.left, where.y-r.top);
		DragMessage(&msg, bmp, B_OP_ALPHA, pt, Window());
		return;
	}
punt:
	Window()->Activate(true);
}

