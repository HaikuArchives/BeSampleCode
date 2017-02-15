// BitmapWriter application class and main()

#include "BitmapWriter.h"
#include "BitmapMovie.h"
#include <interface/Bitmap.h>
#include <interface/Button.h>
#include <interface/MenuBar.h>
#include <interface/MenuItem.h>
#include <interface/Window.h>
#include <interface/PopUpMenu.h>
#include <media/MediaFormats.h>
#include <media/MediaTrack.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// some useful constants
static const int32 MSG_WRITE_FILE = 'Wrfl';
static const int32 MSG_FILE_FORMAT = 'Ffmt';
static const char* MSG_FAMILY_DATA = "BW:family";
static const int32 MSG_CODEC = 'Codc';
static const char* MSG_CODEC_DATA = "BW:codec";

static const float BUTTON_HEIGHT = 30;		// hardcoded GUI metrics
static const float MENU_HEIGHT = 20;
static const float MOVIE_WIDTH = 320;
static const float MOVIE_HEIGHT = 240;
static const float BMAP_WIDTH = MOVIE_WIDTH - 1;		// because pixel ranges include both edges
static const float BMAP_HEIGHT = MOVIE_HEIGHT - 1;	// so for height, [0, 239] is 240 pixels

// BitmapWriter class implementation
BitmapWriter::BitmapWriter(const char *signature) :
	BApplication(signature),
	mFlick(NULL)
{
	mCodec.id = -1;			// no codec selected yet
}

BitmapWriter::~BitmapWriter()
{
	delete mFlick;
}

void 
BitmapWriter::ReadyToRun()
{
	// Create the movie object
	mFlick = new BitmapMovie(MOVIE_WIDTH, MOVIE_HEIGHT, B_RGB32);

	// Set up the bitmap that we'll use as our movie frame buffer
	mBitmap = new BBitmap(BRect(0, 0, BMAP_WIDTH, BMAP_HEIGHT),
		B_BITMAP_ACCEPTS_VIEWS, mFlick->ColorSpace());
	if (!mBitmap->IsValid())
	{
		fprintf(stderr, "ERROR:  BBitmap is not valid\n");
		exit(1);
	}
	if (!mBitmap->Bits())
	{
		fprintf(stderr, "ERROR:  bitmap->Bits() returns NULL\n");
		exit(1);
	}

	// set up the media_format structure corresponding to the BBitmap
	::memset(&mFormat, 0, sizeof(media_format));
	mFormat.type = B_MEDIA_RAW_VIDEO;
	mFormat.u.raw_video.display.line_width = uint32(mFlick->Width());
	mFormat.u.raw_video.display.line_count = uint32(mFlick->Height());
	mFormat.u.raw_video.last_active = mFormat.u.raw_video.display.line_count - 1;
	mFormat.u.raw_video.display.format = mFlick->ColorSpace();
	mFormat.u.raw_video.display.bytes_per_row = mBitmap->BytesPerRow();
	mFormat.u.raw_video.interlace = 1;	
	mFormat.u.raw_video.field_rate = 30;						// frames per second
	mFormat.u.raw_video.pixel_width_aspect = 1;		// square pixels
	mFormat.u.raw_video.pixel_height_aspect = 1;

	// now build the UI
	BRect wb(50, 50, 50 + BMAP_WIDTH + 20, 50 + BMAP_HEIGHT + 20 + MENU_HEIGHT + BUTTON_HEIGHT + 20);
	mWindow = new BWindow(wb, "BitmapWriter", B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS);

	wb = mWindow->Bounds();

	// the "Create Movie" button
	mGoButton = new BButton(BRect(10, wb.bottom - BUTTON_HEIGHT - 10, wb.right - 10, wb.bottom - 10),
		"go", "Create Movie", new BMessage(MSG_WRITE_FILE));
	mWindow->AddChild(mGoButton);
	mGoButton->SetTarget(this);

	// the bitmap view
	BRect b(10, 10, 10 + BMAP_WIDTH, 10 + BMAP_HEIGHT);
	mView = new BView(b, "bitmap view", B_FOLLOW_ALL_SIDES, 0);
	mWindow->AddChild(mView);

	// the file format popup
	b.Set(10, 20 + BMAP_HEIGHT, 5 + BMAP_WIDTH / 2, 20 + BMAP_HEIGHT + MENU_HEIGHT);
	mFormatMenu = new BMenuBar(b, "format", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM,
		B_ITEMS_IN_COLUMN, false);
	BPopUpMenu* m = NULL;
	// fill in the list of available file formats
	media_file_format mff;
	bool foundFormat = false;
	int32 cookie = 0;
	while (get_next_file_format(&cookie, &mff) == B_OK)
	{
		if (mff.capabilities & media_file_format::B_KNOWS_ENCODED_VIDEO)
		{
			BMessage* msg = new BMessage(MSG_FILE_FORMAT);
			msg->AddInt32(MSG_FAMILY_DATA, mff.family);
			BMenuItem* item = new BMenuItem(mff.pretty_name, msg);
			if (!foundFormat)		// make a menu the first time we find a format
			{
				foundFormat = true;
				mFamily = mff.family;
				m = new BPopUpMenu(mff.pretty_name);
				item->SetMarked(true);
			}
			m->AddItem(item);
		}
	}
	mFormatMenu->AddItem(m);
	mWindow->AddChild(mFormatMenu);
	m->SetTargetForItems(this);
	mFormatMenu->SetTargetForItems(this);

	// the codec popup
	b.Set(15 + BMAP_WIDTH / 2, 20 + BMAP_HEIGHT, 10 + BMAP_WIDTH, 20 + BMAP_HEIGHT + MENU_HEIGHT);
	mCodecMenu = new BMenuBar(b, "codec", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM,
		B_ITEMS_IN_COLUMN, false);
	mWindow->AddChild(mCodecMenu);

	// Fill in the codec popup appropriately for the currently-selected file format
	BuildCodecMenu(mFamily);

	// done!
	mWindow->Show();
}

void 
BitmapWriter::MessageReceived(BMessage *msg)
{
	status_t err;
	media_format_family family;
	media_codec_info* p;
	ssize_t size;

	switch (msg->what)
	{
	case MSG_WRITE_FILE:
		mView->Window()->Lock();
		mGoButton->SetEnabled(false);
		mFormatMenu->SetEnabled(false);
		mCodecMenu->SetEnabled(false);
		mView->Window()->Unlock();

		err = WriteFile(mFamily, mCodec, "test_movie");

		mView->Window()->Lock();
		mGoButton->SetEnabled(true);
		mFormatMenu->SetEnabled(true);
		mCodecMenu->SetEnabled(true);
		mView->Window()->Unlock();
		break;

	case MSG_FILE_FORMAT:
		family = media_format_family(msg->FindInt32(MSG_FAMILY_DATA));
		if (family != mFamily)
		{
			// remember the new setting and rebuild the codec menu
			mFamily = family;
			BuildCodecMenu(family);
		}
		break;

	case MSG_CODEC:
		err = msg->FindData(MSG_CODEC_DATA, B_SIMPLE_DATA, (const void**) &p, &size);
		if (B_OK == err)
		{
			if (p->id != mCodec.id)		// new codec selected?
			{
				mCodec = *p;
			}
		}
		break;

	default:
		BApplication::MessageReceived(msg);
		break;
	}
}

// Set up the codec menu for a particular media format family
void 
BitmapWriter::BuildCodecMenu(media_format_family mf_family)
{
	// suspend updates while we're rebuilding this menu in order to
	// reduce window flicker and other annoyances
	mWindow->BeginViewTransaction();

	// remember the currently-selected codec
	int32 current = mCodec.id;	// remember the current codec
	mCodec.id = -2;						// magic invalid value

	// find the full media_file_format corresponding to the given format family (e.g. AVI)
	status_t err;
	media_file_format mff;
	int32 cookie = 0;
	while ((err = get_next_file_format(&cookie, &mff)) == B_OK)
	{
		if (mff.family == mf_family) break;
	}

	// hmm, something is desperately wrong -- we couldn't find the "current" format family
	if (err)
	{
		fprintf(stderr, "ERROR:  couldn't find current media format family\n");
		exit(1);
	}

	BPopUpMenu* m = NULL;

	// remove the current contents of the menu
	BMenuItem* item;
	while ((item = mCodecMenu->RemoveItem(int32(0))) != NULL)
	{
		delete item;
	}

	// fill in the list of available codecs for this media format and file family
	media_codec_info codec, firstCodec;
	media_format outFormat;
	bool foundCodec = false;
	cookie = 0;
	while (get_next_encoder(&cookie, &mff, &mFormat, &outFormat, &codec) == B_OK)
	{
		BMessage* msg = new BMessage(MSG_CODEC);
		msg->AddData(MSG_CODEC_DATA, B_SIMPLE_DATA, &codec, sizeof(media_codec_info));
		BMenuItem* item = new BMenuItem(codec.pretty_name, msg);
		if (!foundCodec)
		{
			foundCodec = true;
			firstCodec = codec;
			m = new BPopUpMenu(codec.pretty_name);
		}
		m->AddItem(item);
		if (codec.id == current)
		{
			mCodec = codec;
			item->SetMarked(true);
		}
	}

	// if we didn't find the currently-selected codec in the new list,
	// just pick the first one
	if (mCodec.id == -2)
	{
		item = m->ItemAt(0);
		mCodec = firstCodec;
		if (item) item->SetMarked(true);
	}

	// Put the popup menu into the BMenuBar container
	mCodecMenu->AddItem(m);

	// Make the app object the menu's message target
	m->SetTargetForItems(this);

	// okay, we're done mucking with the menu
	mWindow->EndViewTransaction();
}

// actually produce a movie file
status_t 
BitmapWriter::WriteFile(media_format_family mfFamily, media_codec_info& codec, const char *path)
{
	entry_ref ref;

	// get an entry_ref for the given file path
	status_t err = ::get_ref_for_path(path, &ref);
	if (err) return err;

	// find the media_file_format for the given family
	media_file_format mff;
	int32 cookie = 0;
	while ((err = get_next_file_format(&cookie, &mff)) == B_OK)
	{
		if (mff.family == mfFamily) break;
	}
	if (err)
	{
		fprintf(stderr, "ERROR:  unable to find given media format family\n");
		return err;
	}

	// Create the file, using the given media file format, input media format, and codec
	err = mFlick->CreateFile(ref, mff, mFormat, codec);
	if (err)
	{
		fprintf(stderr, "ERROR:  BitmapMovie::CreateFile() returned 0x%lx (%s)\n", err, strerror(err));
		return err;
	}

	// Adjust the quality before writing any frames; this is optional
	BMediaTrack* t = mFlick->GetTrack();
	t->SetQuality(0.8);

	// attach a view to our (cached) BBitmap so we can draw more easily
	BView* v = new BView(mBitmap->Bounds(), "bitmap view", B_FOLLOW_LEFT | B_FOLLOW_TOP, 0);
	v->SetPenSize(1);
	mBitmap->AddChild(v);

	// now draw some bitmaps and use them as the frames of the movie
	BRect bmb = mBitmap->Bounds();
	for (int i = 0; i < 256; i++)
	{
		mBitmap->Lock();

		// write a solid grey field into the bitmap
		v->SetLowColor(255-i, 255-i, 255-i);
		v->FillRect(bmb, B_SOLID_LOW);

		// draw a little into the view
		float xrad = bmb.Width() / 2;
		float yrad = bmb.Height() / 2;
		v->SetHighColor(255, 255-i, 255-i);
		v->FillArc(BPoint(xrad, yrad), yrad, yrad, 90, -(360.0 / 255.0 * i), B_SOLID_HIGH);
		v->SetHighColor(255-i, 255-i, 255);
		v->FillArc(BPoint(xrad, yrad), yrad/2, yrad/2, 90, 360.0 / 255.0 * i, B_SOLID_HIGH);

		// we have to sync the view now, otherwise the drawing operations might not be
		// committed before we write the frame to the movie file
		v->Sync();
		mBitmap->Unlock();

		// draw the bitmap into the window so we can watch the animation we're writing;
		// this is not really necessary, but it's handy for purposes of demonstration
		mView->Window()->Lock();
		BRect r = mView->Bounds();
		mView->DrawBitmap(mBitmap, r);
		mView->Window()->Unlock();

		// write the bitmap as one frame of video, locking the bitmap for safety's sake.
		mBitmap->LockBits();
		err = mFlick->WriteFrame(mBitmap, true);
		mBitmap->UnlockBits();
		if (err)
		{
			fprintf(stderr, "ERROR:  WriteFrame() returned 0x%lx (%s)\n", err, strerror(err));
			break;
		}
	}

	// we're done with the view, so get rid of it
	mBitmap->RemoveChild(v);
	delete v;		// this is okay because it isn't a child view any more

	status_t closeErr = mFlick->CloseFile();
	return (err) ? err : closeErr;		// if something failed earlier, return that error
}

// simple little main() function
int main(int, char**)
{
	BitmapWriter app("application/x-vnd.Be-BitmapWriter");
	app.Run();

	return 0;
}
