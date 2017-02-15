// BitmapMovie.h

#ifndef BitmapMovie_H
#define BitmapMovie_H 1

#include <interface/GraphicsDefs.h>
#include <storage/Entry.h>
#include <media/MediaDefs.h>

class BBitmap;
class BMediaFile;
class BMediaTrack;
struct media_codec_info;
struct entry_ref;
class BFile;

class BitmapMovie
{
public:
	BitmapMovie(float width, float height, color_space);
	~BitmapMovie();

	float Width() const { return mWidth; }
	float Height() const { return mHeight; }
	color_space ColorSpace() const { return mColorSpace; }

	// Create the file
	status_t CreateFile(
		const entry_ref& file,
		const media_file_format& file_format,
		const media_format& input_format,
		const media_codec_info& codec,
		float quality = -1);			// [0,1]; negative number means use default quality

	// Write a BBitmap as one frame of the file
	status_t WriteFrame(BBitmap* bitmap, bool isKeyFrame = false);

	// Close down the file nicely
	status_t CloseFile();

	// provide a pointer to the BMediaTrack -- dangerous!
	BMediaTrack* GetTrack() const;

private:
	float mWidth, mHeight;
	color_space mColorSpace;
	BMediaFile* mMediaFile;
	BMediaTrack* mTrack;
	bool mIsOpen, mHeaderCommitted;
};

#endif
