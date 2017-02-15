// BitmapWriter.h

#ifndef BitmapWriter_H
#define BitmapWriter_H 1

#include <app/Application.h>
#include <media/MediaDefs.h>
#include <media/MediaFormats.h>

class BitmapMovie;
class BWindow;
class BBitmap;
class BButton;
class BMenuBar;

// actual BApplication class
class BitmapWriter : public BApplication
{
public:
	BitmapWriter(const char* signature);
	~BitmapWriter();

	void ReadyToRun();
	void MessageReceived(BMessage* msg);

private:
	void BuildCodecMenu(media_format_family mff);
	status_t WriteFile(media_format_family mff, media_codec_info& codec, const char* path);

	BitmapMovie* mFlick;
	BBitmap* mBitmap;
	BWindow* mWindow;
	BView* mView;
	BButton* mGoButton;
	BMenuBar* mFormatMenu;
	BMenuBar* mCodecMenu;
	media_format mFormat;				// format of data from our BBitmaps
	media_format_family mFamily;	// currently selected file format family, e.g. B_AVI_FORMAT_FAMILY
	media_codec_info mCodec;			// currently selected codec
};

#endif
