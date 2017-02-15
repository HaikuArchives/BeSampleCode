//******************************************************************************

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include <Alert.h>
#include <Bitmap.h>
#include <Debug.h>
#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <Path.h>
#include <Screen.h>
#include <ScrollView.h>
#include <TextView.h>
#include <PopUpMenu.h>
#include <Clipboard.h>

#include "main.h"

#if __MWERKS__
#define _UNUSED(x)
#endif

#if __GNUC__
#define _UNUSED(x) x
#endif

static const char *kPrefsFileName = "Magnify_prefs";

const int32 msg_help = 'help';
const int32 msg_update_info = 'info';
const int32 msg_show_info = 'show';
const int32 msg_toggle_grid = 'grid';
const int32 msg_shrink = 'shnk';
const int32 msg_grow = 'grow';
const int32 msg_make_square = 'sqar';
const int32 msg_shrink_pixel = 'pshk';
const int32 msg_grow_pixel = 'pgrw';

const int32 msg_new_color = 'colr';
const int32 msg_toggle_ruler = 'rulr';
const int32 msg_copy_image = 'copy';
const int32 msg_track_color = 'trak';
const int32 msg_freeze = 'frez';
const int32 msg_dump = 'dump';
const int32 msg_add_cross_hair = 'acrs';
const int32 msg_remove_cross_hair = 'rcrs';

#ifndef B_BEOS_VERSION_4
#define PR31
#endif

#ifdef PR31
extern void _get_screen_bitmap_(BBitmap *offscreen,BRect src, bool enable = TRUE);
#endif

//******************************************************************************

static float
FontHeight(BView* target, bool full)
{
	font_height finfo;		
	target->GetFontHeight(&finfo);
	float h = ceil(finfo.ascent) + ceil(finfo.descent);

	if (full)
		h += ceil(finfo.leading);
	
	return h;
}

static color_map*
ColorMap()
{
	color_map* cmap;
	
	BScreen screen(B_MAIN_SCREEN_ID);
	cmap = (color_map*)screen.ColorMap();
	
	return cmap;
}

static void
CenterWindowOnScreen(BWindow* w)
{
	BRect	screenFrame = (BScreen(B_MAIN_SCREEN_ID).Frame());
	BPoint 	pt;
	pt.x = screenFrame.Width()/2 - w->Bounds().Width()/2;
	pt.y = screenFrame.Height()/2 - w->Bounds().Height()/2;

	if (screenFrame.Contains(pt))
		w->MoveTo(pt);
}

//******************************************************************************

int
main(long argc, char* argv[])
{
	int32 pixelCount=-1;

	if (argc > 2) {
		printf("usage: magnify [size] (magnify size * size pixels)\n");
		exit(1);
	} else {
		if (argc == 2) {
			pixelCount = abs(atoi(argv[1]));
	
			if ((pixelCount > 100) || (pixelCount < 4)) {
				printf("usage: magnify [size] (magnify size * size pixels)\n");
				printf("  size must be > 4 and a multiple of 4\n");
				exit(1);
			}
		
			if (pixelCount % 4) {
				printf("magnify: size must be a multiple of 4\n");
				exit(1);
			}
		}
	}

	TApp app(pixelCount);
	app.Run();	
	return(0);
}

// **************************************************************************

//	pass in pixelCount to maintain backward compatibility of setting
//	the pixelcount from the command line
TApp::TApp(int32 pixelCount)
	:BApplication("application/x-vnd.Be-MAGN")
{
	TWindow* magWindow = new TWindow(pixelCount);
	magWindow->Show();
}

void
TApp::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case B_ABOUT_REQUESTED:
			AboutRequested();
			break;
		default:
			BApplication::MessageReceived(msg);
			break;
	}
}

void
TApp::ReadyToRun()
{
	BApplication::ReadyToRun();
}

void
TApp::AboutRequested(void)
{
	(new BAlert("", "Magnify!  Now with even more features.", "OK"))->Go();
}

//******************************************************************************

//	each info region will be:
//	top-bottom: 5 fontheight 5 fontheight 5
//	left-right: 10 minwindowwidth 10

const int32 kBorderSize = 10;

TWindow::TWindow(int32 pixelCount)
	: BWindow( BRect(0,0,0,0), "Magnify", B_TITLED_WINDOW,
		B_OUTLINE_RESIZE)
{
	GetPrefs(pixelCount);

	//	add info view
	BRect infoRect(Bounds());
	infoRect.InsetBy(-1, -1);
	fInfo = new TInfoView(infoRect);
	AddChild(fInfo);
	
	fFontHeight = FontHeight(fInfo, true);
	fInfoHeight = (fFontHeight * 2) + (3 * 5);

	BRect fbRect(0, 0, (fHPixelCount*fPixelSize), (fHPixelCount*fPixelSize));
	if (InfoIsShowing())
		fbRect.OffsetBy(10, fInfoHeight);
	fFatBits = new TMagnify(fbRect, this);
	fInfo->AddChild(fFatBits);	

	fFatBits->SetSelection(fShowInfo);
	fInfo->SetMagView(fFatBits);

	ResizeWindow(fHPixelCount, fVPixelCount);
	
	AddShortcut('I', B_COMMAND_KEY, new BMessage(B_ABOUT_REQUESTED));
	AddShortcut('S', B_COMMAND_KEY, new BMessage(msg_save));
	AddShortcut('C', B_COMMAND_KEY, new BMessage(msg_copy_image));
	AddShortcut('T', B_COMMAND_KEY, new BMessage(msg_show_info));
	AddShortcut('H', B_COMMAND_KEY, new BMessage(msg_add_cross_hair));
	AddShortcut('H', B_SHIFT_KEY, 	new BMessage(msg_remove_cross_hair));
	AddShortcut('G', B_COMMAND_KEY, new BMessage(msg_toggle_grid));
	AddShortcut('F', B_COMMAND_KEY, new BMessage(msg_freeze));
	AddShortcut('-', B_COMMAND_KEY, new BMessage(msg_shrink));
	AddShortcut('=', B_COMMAND_KEY, new BMessage(msg_grow));
	AddShortcut('/', B_COMMAND_KEY, new BMessage(msg_make_square));
	AddShortcut(',', B_COMMAND_KEY, new BMessage(msg_shrink_pixel));
	AddShortcut('.', B_COMMAND_KEY, new BMessage(msg_grow_pixel));
}

TWindow::~TWindow()
{
}

void
TWindow::MessageReceived(BMessage* m)
{
	bool active = fFatBits->Active();
	
	switch (m->what) {
		case B_ABOUT_REQUESTED:
			be_app->MessageReceived(m);
			break;
			
		case msg_help:
			ShowHelp();
			break;
			
		case msg_show_info:
			if (active)
				ShowInfo(!fShowInfo);
			break;
			
		case msg_toggle_grid:
			if (active)
				SetGrid(!fShowGrid);
			break;
			
		case msg_grow:
			if (active)
				ResizeWindow(true);
			break;
		case msg_shrink:
			if (active)
				ResizeWindow(false);
			break;
		case msg_make_square:
			if (active) {
				if (fHPixelCount == fVPixelCount)
					break;
				int32 big = (fHPixelCount > fVPixelCount) ? fHPixelCount : fVPixelCount;
				ResizeWindow(big, big);
			}
			break;
			
		case msg_shrink_pixel:
			if (active)
				SetPixelSize(false);
			break;
		case msg_grow_pixel:
			if (active)
				SetPixelSize(true);
			break;
			
		case msg_add_cross_hair:
			if (active && fShowInfo)
				AddCrossHair();
			break;
		case msg_remove_cross_hair:
			if (active && fShowInfo)
				RemoveCrossHair();
			break;

		case msg_freeze:
			if (active)
				SetFlags(B_OUTLINE_RESIZE | B_NOT_ZOOMABLE | B_NOT_RESIZABLE);
			else
				SetFlags(B_OUTLINE_RESIZE | B_NOT_ZOOMABLE);

			fFatBits->MakeActive(!fFatBits->Active());
			break;

		case msg_save:
			// freeze the image here, unfreeze after dump or cancel
			fFatBits->StartSave();
			
			fSavePanel = new BFilePanel(B_SAVE_PANEL, new BMessenger(NULL, this),
				0, 0, false, new BMessage(msg_dump));
			fSavePanel->SetSaveText("Bitmaps.h");
			fSavePanel->Show();
			break;
		case msg_dump:
			{
				delete fSavePanel;
				
				entry_ref dirRef;
				char* name;
				m->FindRef("directory", &dirRef);
				m->FindString((const char*)"name",(const char**) &name);
				
				fFatBits->SaveImage(&dirRef, name);
			}
			break;
		case B_CANCEL:
			//	image is frozen before the FilePanel is shown
			fFatBits->EndSave();
			break;
			
		case msg_copy_image:
			fFatBits->CopyImage();
			break;
		default:
			BWindow::MessageReceived(m);
			break;
	}	
}

bool
TWindow::QuitRequested()
{
	SetPrefs();
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

// 	prefs are:
//		name = Magnify
//		version
//		show grid
//		show info	(rgb, location)
//		pixel count
//		pixel size
const char* const kAppName = "Magnify";
const bool kDefaultShowGrid = true;
const bool kDefaultShowInfo = true;
const int32 kDefaultPixelCount = 32;
const int32 kDefaultPixelSize = 8;
void
TWindow::GetPrefs(int32 overridePixelCount)
{
	BPath path;
	char name[8];
	float version;
	bool haveLoc=false;
	BPoint loc;
	bool showGrid = kDefaultShowGrid;
	bool showInfo = kDefaultShowInfo;
	bool ch1Showing=false;
	bool ch2Showing=false;
	int32 hPixelCount = kDefaultPixelCount;
	int32 vPixelCount = kDefaultPixelCount;
	int32 pixelSize = kDefaultPixelSize;
	
	if (find_directory (B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		int ref = -1;
		path.Append(kPrefsFileName);
		if ((ref = open(path.Path(), 0)) >= 0) {
		
			if (read(ref, name, 7) != 7)
				goto ALMOST_DONE;
				
			name[7] = 0;
			if (strcmp(name, kAppName) != 0)
				goto ALMOST_DONE;
			
			read(ref, &version, sizeof(float));
			
			if (read(ref, &loc, sizeof(BPoint)) != sizeof(BPoint))
				goto ALMOST_DONE;
			else
				haveLoc = true;
			
			if (read(ref, &showGrid, sizeof(bool)) != sizeof(bool)) {
				showGrid = kDefaultShowGrid;
				goto ALMOST_DONE;
			}

			if (read(ref, &showInfo, sizeof(bool)) != sizeof(bool)) {
				showInfo = kDefaultShowInfo;
				goto ALMOST_DONE;
			}

			if (read(ref, &ch1Showing, sizeof(bool)) != sizeof(bool)) {
				ch1Showing = false;
				goto ALMOST_DONE;
			}

			if (read(ref, &ch2Showing, sizeof(bool)) != sizeof(bool)) {
				ch2Showing = false;
				goto ALMOST_DONE;
			}

			if (read(ref, &hPixelCount, sizeof(int32)) != sizeof(int32)) {
				hPixelCount = kDefaultPixelCount;
				goto ALMOST_DONE;
			}
			if (read(ref, &vPixelCount, sizeof(int32)) != sizeof(int32)) {
				vPixelCount = kDefaultPixelCount;
				goto ALMOST_DONE;
			}

			if (read(ref, &pixelSize, sizeof(int32)) != sizeof(int32)) {
				pixelSize = kDefaultPixelSize;
				goto ALMOST_DONE;
			}

ALMOST_DONE:	//	clean up and try to position the window
			close(ref);

			if (haveLoc && BScreen(B_MAIN_SCREEN_ID).Frame().Contains(loc)) {
				MoveTo(loc);
				goto DONE;
			}			
		}
	}

	// 	if prefs dont yet exist or the window is not onscreen, center the window
	CenterWindowOnScreen(this);

	//	set all the settings to defaults if we get here
DONE:
	fShowGrid = showGrid;
	fShowInfo = showInfo;
	fHPixelCount = (overridePixelCount == -1) ? hPixelCount : overridePixelCount;
	fVPixelCount = (overridePixelCount == -1) ? vPixelCount : overridePixelCount;
	fPixelSize = pixelSize;
}

const float kCurrentVersion = 1.2;
void
TWindow::SetPrefs()
{
	BPath path;

	if (find_directory (B_USER_SETTINGS_DIRECTORY, &path, true) == B_OK) {
		long ref;
		
		path.Append (kPrefsFileName);
		if ((ref = creat(path.Path(), O_RDWR)) >= 0) {
			float version = kCurrentVersion;
			
			lseek (ref, 0, SEEK_SET);
			write(ref, kAppName, 7);
			write(ref, &version, sizeof(float));
			
			BPoint loc = Frame().LeftTop();
			write(ref, &loc, sizeof(BPoint));
			
			write(ref, &fShowGrid, sizeof(bool));
			write(ref, &fShowInfo, sizeof(bool));
			bool ch1, ch2;
			CrossHairsShowing(&ch1, &ch2);
			write(ref, &ch1, sizeof(bool));
			write(ref, &ch2, sizeof(bool));
			
			write(ref, &fHPixelCount, sizeof(int32));
			write(ref, &fVPixelCount, sizeof(int32));
			write(ref, &fPixelSize, sizeof(int32));
			
			close(ref);
			
		}
	}
}

void
TWindow::FrameResized(float w, float h)
{
	CalcViewablePixels();
	
	float width;
	float height;
	GetPreferredSize(&width, &height);		
	ResizeTo(width, height);
	
	fFatBits->InitBuffers(fHPixelCount, fVPixelCount, fPixelSize, ShowGrid());
}

void
TWindow::ScreenChanged(BRect screen_size, color_space depth)
{
	BWindow::ScreenChanged(screen_size, depth);
	//	reset all bitmaps
	fFatBits->ScreenChanged(screen_size,depth);
}

void
TWindow::Minimize(bool m)
{
	BWindow::Minimize(m);
}

void
TWindow::Zoom(BPoint _UNUSED(rec_position), float _UNUSED(rec_width), float _UNUSED(rec_height))
{
	if (fFatBits->Active())
		ShowInfo(!fShowInfo);
}

void
TWindow::CalcViewablePixels()
{

	float w = Bounds().Width();
	float h = Bounds().Height();
	
	if (InfoIsShowing()) {
		w -= 20;							// remove the gutter
		h = h-fInfoHeight-10;				// remove info and gutter
	}

	bool ch1, ch2;
	fFatBits->CrossHairsShowing(&ch1, &ch2);	
	if (ch1)
		h -= fFontHeight;
	if (ch2)
		h -= fFontHeight + 5;

	fHPixelCount = (int32)w / fPixelSize;			// calc h pixels
	if (fHPixelCount < 16)
		fHPixelCount = 16;

	fVPixelCount = (int32)h / fPixelSize;			// calc v pixels
	if (fVPixelCount < 4)
		fVPixelCount = 4;
}

void
TWindow::GetPreferredSize(float* width, float* height)
{
	*width = fHPixelCount * fPixelSize;			// calc window width
	*height = fVPixelCount * fPixelSize;		// calc window height
	if (InfoIsShowing()) {
		*width += 20;			
		*height += fInfoHeight + 10;
	}
		
	bool ch1, ch2;
	fFatBits->CrossHairsShowing(&ch1, &ch2);	
	if (ch1)
		*height += fFontHeight;
	if (ch2)
		*height += fFontHeight + 5;		
}

void
TWindow::ResizeWindow(int32 hPixelCount, int32 vPixelCount)
{
	fHPixelCount = hPixelCount;
	fVPixelCount = vPixelCount;
			
	float width, height;
	GetPreferredSize(&width, &height);
	
	ResizeTo(width, height);
}

void
TWindow::ResizeWindow(bool direction)
{
	int32 x = fHPixelCount;
	int32 y = fVPixelCount;
	
	if (direction) {
		x += 4;
		y += 4;
	} else {
		x -= 4;
		y -= 4;
	}

	if (x < 4)
		x = 4;

	if (y < 4)
		y = 4;

	ResizeWindow(x, y);	
}

void
TWindow::SetGrid(bool s)
{
	if (s == fShowGrid)
		return;
	
	fShowGrid = s;
	fFatBits->SetUpdate(true);
}

bool
TWindow::ShowGrid()
{
	return fShowGrid;
}

void
TWindow::ShowInfo(bool i)
{
	if (i == fShowInfo)
		return;
		
	fShowInfo = i;

	if (fShowInfo)
		fFatBits->MoveTo(10, fInfoHeight);
	else {
		fFatBits->MoveTo(1,1);
		fFatBits->SetCrossHairsShowing(false, false);
	}

	fFatBits->SetSelection(fShowInfo);
	ResizeWindow(fHPixelCount, fVPixelCount);
}

bool
TWindow::InfoIsShowing()
{
	return fShowInfo;
}

void
TWindow::UpdateInfo()
{
	fInfo->Draw(fInfo->Bounds());
}

void
TWindow::AddCrossHair()
{
	fFatBits->AddCrossHair();
	
	//	crosshair info needs to be added
	//	window resizes accordingly		
	float width;
	float height;
	GetPreferredSize(&width, &height);
	ResizeTo(width, height);
}

void
TWindow::RemoveCrossHair()
{
	fFatBits->RemoveCrossHair();
			
	//	crosshair info needs to be removed
	//	window resizes accordingly		
	float width;
	float height;
	GetPreferredSize(&width, &height);
	ResizeTo(width, height);
}

void
TWindow::CrossHairsShowing(bool* ch1, bool* ch2)
{
	fFatBits->CrossHairsShowing(ch1, ch2);
}

void
TWindow::PixelCount(int32* h, int32 *v)
{
	*h = fHPixelCount;
	*v = fVPixelCount;
}

void
TWindow::SetPixelSize(int32 s)
{
	if (s == fPixelSize)
		return;
		
	fPixelSize = s;
	// resize window
	// tell info that size has changed
	// tell mag that size has changed

	CalcViewablePixels();
	ResizeWindow(fHPixelCount, fVPixelCount);
}

void
TWindow::SetPixelSize(bool d)
{
	if (d) {		// grow
		fPixelSize++;
		if (fPixelSize > 16)
			fPixelSize = 16;
	} else {
		fPixelSize--;
		if (fPixelSize < 1)
			fPixelSize = 1;
	}

	float w = Bounds().Width();
	float h = Bounds().Height();
	CalcViewablePixels();
	ResizeWindow(fHPixelCount, fVPixelCount);
	
	//	the window might not actually change in size
	//	in that case force the buffers to the new dimension
	if (w == Bounds().Width() && h == Bounds().Height())
		fFatBits->InitBuffers(fHPixelCount, fVPixelCount, fPixelSize, ShowGrid());
}

int32
TWindow::PixelSize()
{
	return fPixelSize;
}

void
TWindow::ShowHelp()
{
	BRect r(0,0,375,240);
	BWindow* w = new BWindow(r, "Magnify Help", B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_RESIZABLE);
	
	r.right -= B_V_SCROLL_BAR_WIDTH;
	r.bottom -= B_H_SCROLL_BAR_HEIGHT;
	BRect r2(r);
	r2.InsetBy(4,4);
	BTextView* text = new BTextView(r, "text", r2, B_FOLLOW_ALL, B_WILL_DRAW);
	text->MakeSelectable(false);
	text->MakeEditable(false);
	
	BScrollView* scroller = new BScrollView("", text, B_FOLLOW_ALL, 0, true, true);
	w->AddChild(scroller);
	
	text->Insert("General:\n");
	text->Insert("  32 x 32 - the top left numbers are the number of visible\n");
	text->Insert("    pixels (width x height)\n");
	text->Insert("  8 pixels/pixel - represents the number of pixels that are\n");
	text->Insert("    used to magnify a pixel\n");
	text->Insert("  R:152 G:52 B:10 -  the RGB values for the pixel under\n");
	text->Insert("    the red square\n");
	text->Insert("\n\n");
	text->Insert("Copy/Save:\n");
	text->Insert("  copy - copies the current image to the clipboard\n");
	text->Insert("  save - prompts the user for a file to save to and writes out\n");
	text->Insert("    the bits of the image\n");
	text->Insert("\n\n");
	text->Insert("Info:\n");
	text->Insert("  hide/show info - hides/shows all these new features\n");
	text->Insert("    note: when showing, a red square will appear which signifies\n");
	text->Insert("      which pixel's rgb values will be displayed\n");      		
	text->Insert("  add/remove crosshairs - 2 crosshairs can be added (or removed)\n");
	text->Insert("    to aid in the alignment and placement of objects.\n");
	text->Insert("    The crosshairs are represented by blue squares and blue lines.\n");
	text->Insert("  hide/show grid - hides/shows the grid that separates each pixel\n");
	text->Insert("\n\n");
	text->Insert("  freeze - freezes/unfreezes magnification of whatever the\n");
	text->Insert("    cursor is currently over\n");
	text->Insert("\n\n");
	text->Insert("Sizing & Resizing:\n");
	text->Insert("  make square - sets the width and the height to the larger\n");
	text->Insert("    of the two making a square image\n");
	text->Insert("  increase/decrease window size - grows or shrinks the window\n");
	text->Insert("    size by 4 pixels.\n");
	text->Insert("    note: this window can also be resized to any size via the\n");
	text->Insert("      resizing region of the window\n");
	text->Insert("  increase/decrease pixel size - increases or decreases the number\n");
	text->Insert("    of pixels used to magnify a 'real' pixel. Range is 1 to 16.\n");
	text->Insert("\n\n");
	text->Insert("Navigation:\n");
	text->Insert("  arrow keys - move the current selection (rgb indicator or crosshair)\n");
	text->Insert("    around 1 pixel at a time\n");
	text->Insert("  option-arrow key - moves the mouse location 1 pixel at a time\n");
	text->Insert("  x marks the selection - the current selection has an 'x' in it\n");
	
	CenterWindowOnScreen(w);	
	w->Show();
}

bool
TWindow::IsActive()
{
	return fFatBits->Active();
}

//******************************************************************************

TInfoView::TInfoView(BRect frame)
	: BBox(frame, "rgb", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_FRAME_EVENTS,
		B_NO_BORDER)
{
	SetFont(be_plain_font);
	fFontHeight = FontHeight(this, true);	
	fMagView = NULL;
	
	fSelectionColor = kBlack;
	fCH1Loc.x = fCH1Loc.y = fCH2Loc.x = fCH2Loc.y = 0;
	
	fInfoStr[0] = 0;
	fRGBStr[0] = 0;
	fCH1Str[0] = 0;
	fCH2Str[0] = 0;
}

TInfoView::~TInfoView()
{
}

void
TInfoView::AttachedToWindow()
{
	BBox::AttachedToWindow();
	
	dynamic_cast<TWindow*>(Window())->PixelCount(&fHPixelCount, &fVPixelCount);
	fPixelSize = dynamic_cast<TWindow*>(Window())->PixelSize();	

	AddMenu();	
}

void 
TInfoView::Draw(BRect updateRect)
{
	PushState();
	
	char str[64];

	SetLowColor(ViewColor());
	
	BRect invalRect;
	
	int32 hPixelCount, vPixelCount;
	dynamic_cast<TWindow*>(Window())->PixelCount(&hPixelCount, &vPixelCount);
	int32 pixelSize = dynamic_cast<TWindow*>(Window())->PixelSize();
	
	MovePenTo(10, fFontHeight+5);
	sprintf(str, "%li x %li  @ %li pixels/pixel", hPixelCount, vPixelCount,
		pixelSize);
	invalRect.Set(10, 5, 10 + StringWidth(fInfoStr), fFontHeight+7);
	SetHighColor(ViewColor());
	FillRect(invalRect);
	SetHighColor(0,0,0,255);
	strcpy(fInfoStr,str);
	DrawString(fInfoStr);
	
	rgb_color c = { 0,0,0, 255 };
	uchar index = 0;
	if (fMagView) {
		c = fMagView->SelectionColor();
		BScreen s;
		index = s.IndexForColor(c);
	}
	MovePenTo(10, fFontHeight*2+5);
	sprintf(str, "R: %i G: %i B: %i  (0x%x)",
		c.red, c.green, c.blue, index);
	invalRect.Set(10, fFontHeight+7, 10 + StringWidth(fRGBStr), fFontHeight*2+7);
	SetHighColor(ViewColor());
	FillRect(invalRect);
	SetHighColor(0,0,0,255);
	strcpy(fRGBStr,str);
	DrawString(fRGBStr);
		
	bool ch1Showing, ch2Showing;
	dynamic_cast<TWindow*>(Window())->CrossHairsShowing(&ch1Showing, &ch2Showing);
	
	if (fMagView) {
		BPoint pt1(fMagView->CrossHair1Loc());
		BPoint pt2(fMagView->CrossHair2Loc());
		
		float h = Bounds().Height();
		if (ch2Showing) {
			MovePenTo(10, h-12);
			sprintf(str, "2) x: %li y: %li   y: %i", (int32)pt2.x, (int32)pt2.y,
				abs((int)(pt1.y - pt2.y)));
			invalRect.Set(10, h-12-fFontHeight, 10 + StringWidth(fCH2Str), h-10);
			SetHighColor(ViewColor());
			FillRect(invalRect);
			SetHighColor(0,0,0,255);
			strcpy(fCH2Str,str);
			DrawString(fCH2Str);
		}
	
		if (ch1Showing && ch2Showing) {
			MovePenTo(10, h-10-fFontHeight-2);
			sprintf(str, "1) x: %li  y: %li   x: %i", (int32)pt1.x, (int32)pt1.y,
				abs((int)(pt1.x - pt2.x)));
			invalRect.Set(10, h-10-2*fFontHeight-2, 10 + StringWidth(fCH1Str), h-10-fFontHeight);
			SetHighColor(ViewColor());
			FillRect(invalRect);
			SetHighColor(0,0,0,255);
			strcpy(fCH1Str,str);
			DrawString(fCH1Str);
		} else if (ch1Showing) {
			MovePenTo(10, h-10);
			sprintf(str, "x: %li  y: %li", (int32)pt1.x, (int32)pt1.y);
			invalRect.Set(10, h-10-fFontHeight, 10 + StringWidth(fCH1Str), h-8);
			SetHighColor(ViewColor());
			FillRect(invalRect);
			SetHighColor(0,0,0,255);
			strcpy(fCH1Str,str);
			DrawString(fCH1Str);
		}
	}
	
	PopState();
}

void
TInfoView::FrameResized(float width, float height)
{
	BBox::FrameResized(width, height);
}

static void
BuildInfoMenu(BMenu *menu)
{
	BMenuItem* menuItem;
	
	menuItem = new BMenuItem("About Magnify...", new BMessage(B_ABOUT_REQUESTED));
	menu->AddItem(menuItem);
	menuItem = new BMenuItem("Help...", new BMessage(msg_help));
	menu->AddItem(menuItem);
	menu->AddSeparatorItem();

	menuItem = new BMenuItem("Save Image", new BMessage(msg_save),'S');
	menu->AddItem(menuItem);
//	menuItem = new BMenuItem("Save Selection", new BMessage(msg_save),'S');
//	menu->AddItem(menuItem);
	menuItem = new BMenuItem("Copy Image", new BMessage(msg_copy_image),'C');
	menu->AddItem(menuItem);
	menu->AddSeparatorItem();

	menuItem = new BMenuItem("Hide/Show Info", new BMessage(msg_show_info),'T');
	menu->AddItem(menuItem);
	menuItem = new BMenuItem("Add a Crosshair", new BMessage(msg_add_cross_hair),'H');
	menu->AddItem(menuItem);
	menuItem = new BMenuItem("Remove a Crosshair", new BMessage(msg_remove_cross_hair), 'H',
		B_SHIFT_KEY);
	menu->AddItem(menuItem);
	menuItem = new BMenuItem("Hide/Show Grid", new BMessage(msg_toggle_grid),'G');
	menu->AddItem(menuItem);
	menu->AddSeparatorItem();

	menuItem = new BMenuItem("Freeze/Unfreeze image", new BMessage(msg_freeze),'F');
	menu->AddItem(menuItem);
	menu->AddSeparatorItem();
	
	menuItem = new BMenuItem("Make Square", new BMessage(msg_make_square),'/');
	menu->AddItem(menuItem);
	menuItem = new BMenuItem("Decrease Window Size", new BMessage(msg_shrink),'-');
	menu->AddItem(menuItem);
	menuItem = new BMenuItem("Increase Window Size", new BMessage(msg_grow),'+');
	menu->AddItem(menuItem);
	menuItem = new BMenuItem("Decrease Pixel Size", new BMessage(msg_shrink_pixel),',');
	menu->AddItem(menuItem);
	menuItem = new BMenuItem("Increase Pixel Size", new BMessage(msg_grow_pixel),'.');
	menu->AddItem(menuItem);	
}

void
TInfoView::AddMenu()
{
	fMenu = new TMenu(dynamic_cast<TWindow*>(Window()), "");
	BuildInfoMenu(fMenu);

	BRect r(Bounds().Width()-27, 11, Bounds().Width()-11, 27);
	fPopUp = new BMenuField( r, "region menu", NULL, fMenu, true,
		B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	AddChild(fPopUp);
}

void
TInfoView::SetMagView(TMagnify* magView)
{
	fMagView = magView;
}

//


TMenu::TMenu(TWindow *mainWindow, const char *title, menu_layout layout)
	: BMenu(title, layout),
		fMainWindow(mainWindow)
{
}


TMenu::~TMenu()
{
}

void 
TMenu::AttachedToWindow()
{
	bool state=true;
	if (fMainWindow)
		state = fMainWindow->IsActive();
	
	BMenuItem* menuItem = FindItem("Hide/Show Info");
	if (menuItem)
		menuItem->SetEnabled(state);
	menuItem = FindItem("Add a Crosshair");
	if (menuItem)
		menuItem->SetEnabled(state);
	menuItem = FindItem("Remove a Crosshair");
	if (menuItem)
		menuItem->SetEnabled(state);
	menuItem = FindItem("Hide/Show Grid");
	if (menuItem)
		menuItem->SetEnabled(state);
	menuItem = FindItem("Make Square");
	if (menuItem)
		menuItem->SetEnabled(state);
	menuItem = FindItem("Decrease Window Size");
	if (menuItem)
		menuItem->SetEnabled(state);
	menuItem = FindItem("Increase Window Size");
	if (menuItem)
		menuItem->SetEnabled(state);
	menuItem = FindItem("Decrease Pixel Size");
	if (menuItem)
		menuItem->SetEnabled(state);
	menuItem = FindItem("Increase Pixel Size");
	if (menuItem)
		menuItem->SetEnabled(state);

	BMenu::AttachedToWindow();
}


//******************************************************************************

TMagnify::TMagnify(BRect r, TWindow* parent)
	:BView(r, "MagView", B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS),
		fParent(parent)
{
	fLastLoc.Set(-1, -1);
	fSelectionLoc.x = 0; fSelectionLoc.y = 0;
	fActive = true;
	fShowSelection = false;
	fShowCrossHair1 = false;
	fCrossHair1.x = -1; fCrossHair1.y = -1;
	fShowCrossHair2 = false;
	fCrossHair2.x = -1; fCrossHair2.y = -1;
	fSelection = -1;

	fImageBuf = NULL;
	fImageView = NULL;
}

TMagnify::~TMagnify()
{
	kill_thread(fThread);
	delete(fImageBuf);
}

void
TMagnify::AttachedToWindow()
{
	SetViewColor(B_TRANSPARENT_32_BIT);

	int32 width, height;
	fParent->PixelCount(&width, &height);
	InitBuffers(width, height, fParent->PixelSize(), fParent->ShowGrid());
	
	fThread = spawn_thread(TMagnify::MagnifyTask, "MagnifyTask",
		B_NORMAL_PRIORITY, this);

	resume_thread(fThread);

	MakeFocus();
}

void
TMagnify::InitBuffers(int32 hPixelCount, int32 vPixelCount,
	int32 pixelSize, bool showGrid)
{
	color_space colorSpace =  BScreen(Window()).ColorSpace();

	BRect r(0, 0, (pixelSize * hPixelCount)-1, (pixelSize * vPixelCount)-1);
	if (Bounds().Width() != r.Width() || Bounds().Height() != r.Height())
		ResizeTo(r.Width(), r.Height());
	
	if (fImageView) {
		fImageBuf->Lock();
		fImageView->RemoveSelf();
		fImageBuf->Unlock();
		
		fImageView->Resize((int32)r.Width(), (int32)r.Height());
		fImageView->SetSpace(colorSpace);
	} else
		fImageView = new TOSMagnify(r, this, colorSpace);

	delete(fImageBuf);
	fImageBuf = new BBitmap(r, colorSpace, true);
	fImageBuf->Lock();
	fImageBuf->AddChild(fImageView);
	fImageBuf->Unlock();
}

void
TMagnify::Draw(BRect)
{
	BRect bounds(Bounds());
	DrawBitmap(fImageBuf, bounds, bounds);
	dynamic_cast<TWindow*>(Window())->UpdateInfo();
}

void
TMagnify::KeyDown(const char *key, int32 numBytes)
{
	if (!fShowSelection)
		BView::KeyDown(key,numBytes);
		
	uint32 mods = modifiers();
	
	switch (key[0]) {
		case B_TAB:
			if (fShowCrossHair1) {
				fSelection++;
				
				if (fShowCrossHair2) {
					if (fSelection > 2)
						fSelection = 0;
				} else if (fShowCrossHair1) {
					if (fSelection > 1)
						fSelection = 0;
				}
				fNeedToUpdate = true;
				Draw(Bounds());
			}
			break;
			
		case B_LEFT_ARROW:
			if (mods & B_OPTION_KEY)
				NudgeMouse(-1,0);
			else
				MoveSelection(-1,0);
			break;
		case B_RIGHT_ARROW:
			if (mods & B_OPTION_KEY)
				NudgeMouse(1, 0);
			else
				MoveSelection(1,0);
			break;
		case B_UP_ARROW:
			if (mods & B_OPTION_KEY)
				NudgeMouse(0, -1);
			else
				MoveSelection(0,-1);
			break;
		case B_DOWN_ARROW:
			if (mods & B_OPTION_KEY)
				NudgeMouse(0, 1);
			else
				MoveSelection(0,1);
			break;
		
		default:
			BView::KeyDown(key,numBytes);
			break;
	}
}

void
TMagnify::FrameResized(float newW, float newH)
{
	int32 w, h;
	PixelCount(&w, &h);
	
	if (fSelectionLoc.x >= w)
		fSelectionLoc.x = 0;
	if (fSelectionLoc.y >= h)
		fSelectionLoc.y = 0;
		
	if (fShowCrossHair1) {
	
		if (fCrossHair1.x >= w) {
			fCrossHair1.x = fSelectionLoc.x + 2;
			if (fCrossHair1.x >= w)
				fCrossHair1.x = 0;
		}
		if (fCrossHair1.y >= h) {
			fCrossHair1.y = fSelectionLoc.y + 2;
			if (fCrossHair1.y >= h)
				fCrossHair1.y = 0;
		}

		if (fShowCrossHair2) {
			if (fCrossHair2.x >= w) {
				fCrossHair2.x = fCrossHair1.x + 2;
				if (fCrossHair2.x >= w)
					fCrossHair2.x = 0;
			}
			if (fCrossHair2.y >= h) {
				fCrossHair2.y = fCrossHair1.y + 2;
				if (fCrossHair2.y >= h)
					fCrossHair2.y = 0;
			}
		}
	}
}

void
TMagnify::MouseDown(BPoint where)
{
	BMessage *currentMsg = Window()->CurrentMessage();
	if (currentMsg->what == B_MOUSE_DOWN) {
		uint32 buttons = 0;
		currentMsg->FindInt32("buttons", (int32 *)&buttons);

		uint32 modifiers = 0;
		currentMsg->FindInt32("modifiers", (int32 *)&modifiers);

		if ((buttons & B_SECONDARY_MOUSE_BUTTON) || (modifiers & B_CONTROL_KEY)) {
			// secondary button was clicked or control key was down, show menu and return

			BPopUpMenu *menu = new BPopUpMenu("Info");
			menu->SetFont(be_plain_font);
			BuildInfoMenu(menu);
			
			BMenuItem *selected = menu->Go(ConvertToScreen(where));
			if (selected)
				Window()->PostMessage(selected->Message()->what);
			return;
		}
	
		//	add a mousedown looper here
	
		int32 pixelSize = PixelSize();	
		float x = where.x / pixelSize;
		float y = where.y / pixelSize;
	
		MoveSelectionTo(x, y);
	
		//	draw the frozen image	
		//	update the info region
		
		fNeedToUpdate = true;
		Draw(Bounds());
	}
}

void
TMagnify::ScreenChanged(BRect, color_space)
{
	int32 width, height;
	fParent->PixelCount(&width, &height);
	InitBuffers(width, height, fParent->PixelSize(), fParent->ShowGrid());
}

void
TMagnify::SetSelection(bool state)
{
	if (fShowSelection == state)
		return;
		
	fShowSelection = state;
	fSelection = 0;
	Draw(Bounds());
}

static void
BoundsSelection(int32 incX, int32 incY, float* x, float* y,
	int32 xCount, int32 yCount)
{
	*x += incX;
	*y += incY;
	
	if (*x < 0)
		*x = xCount-1;
	if (*x >= xCount)
		*x = 0;
		
	if (*y < 0)
		*y = yCount-1;
	if (*y >= yCount)
		*y = 0;
}

void
TMagnify::MoveSelection(int32 x, int32 y)
{
	if (!fShowSelection)
		return;
		
	int32 xCount, yCount;
	PixelCount(&xCount, &yCount);

	float xloc, yloc;
	if (fSelection == 0) {
		xloc = fSelectionLoc.x;
		yloc = fSelectionLoc.y;
		BoundsSelection(x, y, &xloc, &yloc, xCount, yCount);
		fSelectionLoc.x = xloc;
		fSelectionLoc.y = yloc;
	} else if (fSelection == 1) {
		xloc = fCrossHair1.x;
		yloc = fCrossHair1.y;
		BoundsSelection(x, y, &xloc, &yloc, xCount, yCount);
		fCrossHair1.x = xloc;
		fCrossHair1.y = yloc;
	} else if (fSelection == 2) {
		xloc = fCrossHair2.x;
		yloc = fCrossHair2.y;
		BoundsSelection(x, y, &xloc, &yloc, xCount, yCount);
		fCrossHair2.x = xloc;
		fCrossHair2.y = yloc;
	}

	fNeedToUpdate = true;
	Draw(Bounds());
}

void
TMagnify::MoveSelectionTo(int32 x, int32 y)
{
	if (!fShowSelection)
		return;
		
	int32 xCount, yCount;
	PixelCount(&xCount, &yCount);
	if (x >= xCount)
		x = 0;
	if (y >= yCount)
		y = 0;
		
	if (fSelection == 0) {
		fSelectionLoc.x = x;
		fSelectionLoc.y = y;
	} else if (fSelection == 1) {
		fCrossHair1.x = x;
		fCrossHair1.y = y;
	} else if (fSelection == 2) {
		fCrossHair2.x = x;
		fCrossHair2.y = y;
	}

	fNeedToUpdate = true;
	Draw(Bounds());
}

void
TMagnify::ShowSelection()
{
}

short
TMagnify::Selection()
{
	return fSelection;
}

bool
TMagnify::SelectionIsShowing()
{
	return fShowSelection;
}

void
TMagnify::SelectionLoc(float* x, float* y)
{
	*x = fSelectionLoc.x;
	*y = fSelectionLoc.y;
}

void
TMagnify::SetSelectionLoc(float x, float y)
{
	fSelectionLoc.x = x;
	fSelectionLoc.y = y;
}

rgb_color
TMagnify::SelectionColor()
{
	return fImageView->ColorAtSelection();
}

void
TMagnify::CrossHair1Loc(float* x, float* y)
{
	*x = fCrossHair1.x;
	*y = fCrossHair1.y;
}

void
TMagnify::CrossHair2Loc(float* x, float* y)
{
	*x = fCrossHair2.x;
	*y = fCrossHair2.y;
}

BPoint
TMagnify::CrossHair1Loc()
{
	return fCrossHair1;
}

BPoint
TMagnify::CrossHair2Loc()
{
	return fCrossHair2;
}

#include <WindowScreen.h>
void
TMagnify::NudgeMouse(float x, float y)
{
	BPoint		loc;
	ulong		button;

	GetMouse(&loc, &button);
	ConvertToScreen(&loc);
	loc.x += x;
	loc.y += y;

	set_mouse_position((int32)loc.x, (int32)loc.y);
}

void
TMagnify::WindowActivated(bool active)
{
	if (active)
		MakeFocus();
}

long
TMagnify::MagnifyTask(void *arg)
{
	TMagnify*	view = (TMagnify*)arg;

	// static data members can't access members, methods without
	// a pointer to an instance of the class
	TWindow* window = (TWindow*)view->Window();

	while (true) {
		if (window->Lock()) {

//			if (!window->Minimized() && view->Active() || view->NeedToUpdate())
			if (view->NeedToUpdate() || view->Active())
				view->Update(view->NeedToUpdate());

			window->Unlock();
		}
		snooze(35000);
	}

	return B_NO_ERROR;
}

void
TMagnify::Update(bool force)
{
	BPoint		loc;
	ulong		button;
	static long counter = 0;

	GetMouse(&loc, &button);

	ConvertToScreen(&loc);
	if (force || (fLastLoc != loc) || (counter++ % 35 == 0)) {

		if (fImageView->CreateImage(loc, force))
			Draw(Bounds());

		counter = 0;
		if (force)
			SetUpdate(false);

	}
	fLastLoc = loc;
}

bool
TMagnify::NeedToUpdate()
{
	return fNeedToUpdate;
}

void
TMagnify::SetUpdate(bool s)
{
	fNeedToUpdate = s;
}

const char* const kBitmapMimeType = "image/x-vnd.Be-bitmap";

void
TMagnify::CopyImage()
{
	StartSave();
	be_clipboard->Lock();
	be_clipboard->Clear();
	
	BMessage *message = be_clipboard->Data();
	if (!message) {
		printf("no clip msg\n");
		return;
	}
	
	BMessage *embeddedBitmap = new BMessage();
	(fImageView->Bitmap())->Archive(embeddedBitmap,false);
	status_t err = message->AddMessage(kBitmapMimeType, embeddedBitmap);
	ASSERT(err == B_OK);
	err = message->AddRect("rect", (fImageView->Bitmap())->Bounds());
	ASSERT(err == B_OK);

	be_clipboard->Commit();
	be_clipboard->Unlock();
	EndSave();
}

void
TMagnify::AddCrossHair()
{
	if (fShowCrossHair1 && fShowCrossHair2)
		return;
		
	int32 w, h;
	PixelCount(&w, &h);
	
	if (fShowCrossHair1) {
		fSelection = 2;
		fShowCrossHair2 = true;
		fCrossHair2.x = fCrossHair1.x + 2;
		if (fCrossHair2.x >= w)
			fCrossHair2.x = 0;
		fCrossHair2.y = fCrossHair1.y + 2;
		if (fCrossHair2.y >= h)
			fCrossHair2.y = 0;
	} else {
		fSelection = 1;
		fShowCrossHair1 = true;
		fCrossHair1.x = fSelectionLoc.x + 2;
		if (fCrossHair1.x >= w)
			fCrossHair1.x = 0;
		fCrossHair1.y = fSelectionLoc.y + 2;
		if (fCrossHair1.y >= h)
			fCrossHair1.y = 0;
	}
	Draw(Bounds());	
}

void
TMagnify::RemoveCrossHair()
{
	if (!fShowCrossHair1 && !fShowCrossHair2)
		return;
		
	if (fShowCrossHair2) {
		fSelection = 1;
		fShowCrossHair2 = false;
	} else if (fShowCrossHair1) {
		fSelection = 0;
		fShowCrossHair1 = false;
	}
	Draw(Bounds());
}

void
TMagnify::SetCrossHairsShowing(bool ch1, bool ch2)
{
	fShowCrossHair1 = ch1;
	fShowCrossHair2 = ch2;
}

void
TMagnify::CrossHairsShowing(bool* ch1, bool* ch2)
{
	*ch1 = fShowCrossHair1;
	*ch2 = fShowCrossHair2;
}

void
TMagnify::MakeActive(bool s)
{
	fActive = s;
}

void
TMagnify::PixelCount(int32* width, int32* height)
{
	fParent->PixelCount(width, height);
}

int32
TMagnify::PixelSize()
{
	return fParent->PixelSize();
}

bool
TMagnify::ShowGrid()
{
	return fParent->ShowGrid();
}

void
TMagnify::StartSave()
{
	fImageFrozenOnSave = Active();
	if (fImageFrozenOnSave)
		MakeActive(false);
}

void
TMagnify::EndSave()
{
	if (fImageFrozenOnSave)
		MakeActive(true);
}

void
TMagnify::SaveImage(entry_ref* ref, char* name, bool selectionOnly)
{
	//	create a new file
	BFile file;
	BDirectory parentDir(ref);
	parentDir.CreateFile(name, &file);
	
	//	write off the bitmaps bits to the file
	SaveBits(&file, fImageView->Bitmap(), "Data");
	
	// unfreeze the image, image was frozen before invoke of FilePanel
	EndSave();
}

void 
TMagnify::SaveBits(BFile* file, const BBitmap *bitmap, char* name) const
{
	int32 bytesPerPixel;
	const char *kColorSpaceName;	

	switch (bitmap->ColorSpace()) {
		case B_GRAY8:
			bytesPerPixel = 1;
			kColorSpaceName = "B_GRAY8";
			break;
			
		case B_CMAP8:
			bytesPerPixel = 1;
			kColorSpaceName = "B_CMAP8";
			break;
			
		case B_RGB15:
		case B_RGBA15:
		case B_RGB15_BIG:
		case B_RGBA15_BIG:
			bytesPerPixel = 2;
			kColorSpaceName = "B_RGB15";
			break;

		case B_RGB16:
		case B_RGB16_BIG:
			bytesPerPixel = 2;
			kColorSpaceName = "B_RGB16";
			break;
			
		case B_RGB32:
		case B_RGBA32:
		case B_RGBA32_BIG:
		case B_BIG_RGB_32_BIT:
			bytesPerPixel = 3;
			kColorSpaceName = "B_RGB32";
			break;
			
		default:
			printf("dump: usupported ColorSpace\n");
			return;
	}
	
	char str[1024];
	// stream out the width, height and ColorSpace
	sprintf(str, "const int32 k%sWidth = %ld;\n", name, (int32)bitmap->Bounds().Width()+1);
	file->Write(str, strlen(str));
	sprintf(str, "const int32 k%sHeight = %ld;\n", name, (int32)bitmap->Bounds().Height()+1);
	file->Write(str, strlen(str));
	sprintf(str, "const color_space k%sColorSpace = %s;\n\n", name, kColorSpaceName);
	file->Write(str, strlen(str));

	// stream out the constant name for this array
	sprintf(str, "const unsigned char k%sBits [] = {", name);
	file->Write(str, strlen(str));

	const unsigned char *bits = (const unsigned char *)bitmap->Bits();
	const int32 kMaxColumnWidth = 16;
	int32 bytesPerRow = bitmap->BytesPerRow();
	int32 columnWidth = (bytesPerRow < kMaxColumnWidth) ? bytesPerRow : kMaxColumnWidth;
	
	for (int32 remaining = bitmap->BitsLength(); remaining; ) {

		sprintf(str, "\n\t");
		file->Write(str, strlen(str));
		
		//	stream out each row, based on the number of bytes required per row
		//	padding is in the bitmap and will be streamed as 0xff
		for (int32 column = 0; column < columnWidth; column++) {
		
			// stream out individual pixel components
			for (int32 count = 0; count < bytesPerPixel; count++) {
				--remaining;
				sprintf(str, "0x%02x", *bits++);
				file->Write(str, strlen(str));
				
				if (remaining) {
					sprintf(str, ",");
					file->Write(str, strlen(str));
				} else
					break;
			}
			
			//	make sure we don't walk off the end of the bits array
			if (!remaining)
				break;

		}
	}
	sprintf(str, "\n};\n\n");
	file->Write(str, strlen(str));
}

//******************************************************************************

TOSMagnify::TOSMagnify(BRect r, TMagnify* parent, color_space space)
	:BView(r, "ImageView", B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS),
		fColorSpace(space), fParent(parent)
{
	fColorMap = ColorMap();

	switch (space) {
		case B_COLOR_8_BIT:
			fBytesPerPixel = 1;
			break;
		case B_RGB15:
 		case B_RGBA15:
		case B_RGB15_BIG:
		case B_RGBA15_BIG:
		case B_RGB16:
        case B_RGB16_BIG:
			fBytesPerPixel = 2;
			break;
		case B_RGB32:
		case B_RGBA32:
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
			fBytesPerPixel = 4;
			break;
		default:
			// uh, oh -- a color space we don't support
			fprintf(stderr, "Tried to run in an unsupported color space; exiting\n");
			exit(1);
			break;
	}

	fPixel = NULL;
	fBitmap = NULL;
	fOldBits = NULL;
	InitObject();
}

TOSMagnify::~TOSMagnify()
{
	delete fPixel;
	delete(fBitmap);
	free(fOldBits);
}

void TOSMagnify::SetSpace(color_space space)
{
	fColorSpace = space;
	InitObject();
};

void TOSMagnify::InitObject()
{
	int32 w, h;
	fParent->PixelCount(&w, &h);

	if (fBitmap) delete fBitmap;
	BRect bitsRect(0, 0, w-1, h-1);
	fBitmap = new BBitmap(bitsRect, fColorSpace);
	
	if (fOldBits) free(fOldBits);
	fOldBits = (char*)malloc(fBitmap->BitsLength());

	if (!fPixel) {
		#if B_HOST_IS_BENDIAN
		#define native B_RGBA32_BIG
		#else
		#define native B_RGBA32_LITTLE
		#endif
		fPixel = new BBitmap(BRect(0,0,0,0), native, true);
		#undef native
		fPixelView = new BView(BRect(0,0,0,0),NULL,0,0);
		fPixel->Lock();
		fPixel->AddChild(fPixelView);
		fPixel->Unlock();
	};
}

void
TOSMagnify::FrameResized(float width, float height)
{
	BView::FrameResized(width, height);
	InitObject();
}

void
TOSMagnify::Resize(int32 width, int32 height)
{
	ResizeTo(width, height);
	InitObject();
}

bool
TOSMagnify::CreateImage(BPoint mouseLoc, bool force)
{
	bool created = false;
	if (Window() && Window()->Lock()) {
		int32 width, height;
		fParent->PixelCount(&width, &height);
		int32 pixelSize = fParent->PixelSize();
		
		BRect srcRect(0, 0, width - 1, height - 1);
		srcRect.OffsetBy(	mouseLoc.x - (width / 2),
							mouseLoc.y - (height / 2));
		if (force || CopyScreenRect(srcRect)) {

			srcRect.OffsetTo(BPoint(0, 0));
			BRect destRect(Bounds());

			DrawBitmap(fBitmap, srcRect, destRect);

			DrawGrid(width, height, destRect, pixelSize);		
			DrawSelection();

			Sync();
			created = true;
		}
		Window()->Unlock();
	} else
		printf("window problem\n");

	return(created);
}

bool
TOSMagnify::CopyScreenRect(BRect srcRect)
{
	// constrain src rect to legal screen rect
	BScreen screen( Window() );
	BRect scrnframe = screen.Frame();

	if (srcRect.right > scrnframe.right)
		srcRect.OffsetTo(scrnframe.right - srcRect.Width(),
				 		 srcRect.top);
	if (srcRect.top < 0)
		srcRect.OffsetTo(srcRect.left, 0);

	if (srcRect.bottom > scrnframe.bottom)
		srcRect.OffsetTo(srcRect.left,
				 		 scrnframe.bottom - srcRect.Height());
	if (srcRect.left < 0)
		srcRect.OffsetTo(0, srcRect.top);

	// save a copy of the bits for comparison later
	memcpy(fOldBits, fBitmap->Bits(), fBitmap->BitsLength());

#ifdef PR31
	_get_screen_bitmap_(fBitmap, srcRect);
#else
	screen.ReadBitmap(fBitmap, false, &srcRect);
#endif

	// let caller know whether bits have actually changed
	return(memcmp(fBitmap->Bits(), fOldBits, fBitmap->BitsLength()) != 0);
}

void
TOSMagnify::DrawGrid(int32 width, int32 height, BRect destRect, int32 pixelSize)
{
	// draw grid
	if (fParent->ShowGrid() && fParent->PixelSize() > 2) {
		BeginLineArray(width * height);
	
		// horizontal lines
		for (int32 i = pixelSize; i < (height * pixelSize); i += pixelSize)
			AddLine(BPoint(0, i), BPoint(destRect.right, i), kGridGray);
			
		// vertical lines
		for (int32 i = pixelSize; i < (width * pixelSize); i += pixelSize)
			AddLine(BPoint(i, 0), BPoint(i, destRect.bottom), kGridGray);
	
		EndLineArray();
	}
	
	SetHighColor(kGridGray);
	StrokeRect(destRect);
}

void
TOSMagnify::DrawSelection()
{
	if (!fParent->SelectionIsShowing())
		return;

	float x, y;
	int32 pixelSize = fParent->PixelSize();
	int32 squareSize = pixelSize - 2;
	
	fParent->SelectionLoc(&x, &y);
	x *= pixelSize; x++;
	y *= pixelSize; y++;
	BRect selRect(x, y, x+squareSize, y+squareSize);
	
	short selection = fParent->Selection();
	
	PushState();
	SetLowColor(ViewColor());
	SetHighColor(kRedColor);
	StrokeRect(selRect);
	if (selection == 0) {
		StrokeLine(BPoint(x,y), BPoint(x+squareSize,y+squareSize));
		StrokeLine(BPoint(x,y+squareSize), BPoint(x+squareSize,y));
	}	
	
	bool ch1Showing, ch2Showing;
	fParent->CrossHairsShowing(&ch1Showing, &ch2Showing);
	if (ch1Showing) {
		SetHighColor(kBlueColor);
		fParent->CrossHair1Loc(&x, &y);
		x *= pixelSize; x++;
		y *= pixelSize; y++;
		selRect.Set(x, y,x+squareSize, y+squareSize);
		StrokeRect(selRect);
		BeginLineArray(4);
		AddLine(BPoint(0, y+(squareSize/2)),
			BPoint(x, y+(squareSize/2)), kBlueColor);					//	left
		AddLine(BPoint(x+squareSize,y+(squareSize/2)),
			BPoint(Bounds().Width(), y+(squareSize/2)), kBlueColor);	// right
		AddLine(BPoint(x+(squareSize/2), 0),
			BPoint(x+(squareSize/2), y), kBlueColor);					// top
		AddLine(BPoint(x+(squareSize/2), y+squareSize),
			BPoint(x+(squareSize/2), Bounds().Height()), kBlueColor);	// bottom
		EndLineArray();
		if (selection == 1) {
			StrokeLine(BPoint(x,y), BPoint(x+squareSize,y+squareSize));
			StrokeLine(BPoint(x,y+squareSize), BPoint(x+squareSize,y));
		}	
	}
	if (ch2Showing) {
		SetHighColor(kBlueColor);
		fParent->CrossHair2Loc(&x, &y);
		x *= pixelSize; x++;
		y *= pixelSize; y++;
		selRect.Set(x, y,x+squareSize, y+squareSize);
		StrokeRect(selRect);
		BeginLineArray(4);
		AddLine(BPoint(0, y+(squareSize/2)),
			BPoint(x, y+(squareSize/2)), kBlueColor);					//	left
		AddLine(BPoint(x+squareSize,y+(squareSize/2)),
			BPoint(Bounds().Width(), y+(squareSize/2)), kBlueColor);	// right
		AddLine(BPoint(x+(squareSize/2), 0),
			BPoint(x+(squareSize/2), y), kBlueColor);					// top
		AddLine(BPoint(x+(squareSize/2), y+squareSize),
			BPoint(x+(squareSize/2), Bounds().Height()), kBlueColor);	// bottom
		EndLineArray();
		if (selection == 2) {
			StrokeLine(BPoint(x,y), BPoint(x+squareSize,y+squareSize));
			StrokeLine(BPoint(x,y+squareSize), BPoint(x+squareSize,y));
		}	
	}
	
	PopState();
}

rgb_color
TOSMagnify::ColorAtSelection()
{			
	float x, y;
	fParent->SelectionLoc(&x, &y);
	BRect srcRect(x,y,x,y);
	BRect dstRect(0,0,0,0);
	fPixel->Lock();
	fPixelView->DrawBitmap(fBitmap,srcRect,dstRect);
	fPixelView->Sync();
	fPixel->Unlock();

	uint32 pixel = *((uint32*)fPixel->Bits());	
	rgb_color c;
	c.alpha = pixel >> 24;
	c.red = (pixel >> 16) & 0xFF;
	c.green = (pixel >> 8) & 0xFF;
	c.blue = pixel & 0xFF;

	return c;	
}
