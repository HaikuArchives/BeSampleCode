// BitmapMovie

#include "BitmapMovie.h"
#include <interface/Bitmap.h>
#include <storage/File.h>
#include <media/MediaFile.h>
#include <media/MediaTrack.h>

BitmapMovie::BitmapMovie(float width, float height, color_space cSpace) :
	mWidth(width), mHeight(height), mColorSpace(cSpace),
	mMediaFile(NULL), mTrack(NULL), mIsOpen(false), mHeaderCommitted(false)
{
}

BitmapMovie::~BitmapMovie()
{
	// If the movie is still opened, close it; this also flushes all tracks
	if (mMediaFile && mIsOpen) CloseFile();
	delete mMediaFile;		// deletes the track, too
}

status_t 
BitmapMovie::CreateFile(
	const entry_ref& ref,
	const media_file_format& mff,
	const media_format& inputFormat,
	const media_codec_info& mci,
	float quality)
{
	BMediaFile* file = new BMediaFile(&ref, &mff);
	status_t err = file ->InitCheck();
	if (!err)
	{
		mIsOpen = true;
		mHeaderCommitted = false;
		mMediaFile = file;

		// This next line casts away const to avoid a warning.  MediaFile::CreateTrack()
		// *should* have the input format argument declared const, but it doesn't, and
		// it can't be changed because it would break binary compatibility.  Oh, well.
		mTrack = file->CreateTrack((media_format*) &inputFormat, &mci);
		if (!mTrack) err = B_ERROR;
		else
		{
			if (quality >= 0) mTrack->SetQuality(quality);
		}
	}

	// clean up if we incurred an error
	if (err)
	{
		mIsOpen = mHeaderCommitted = false;
		delete mMediaFile;
		mMediaFile = NULL;
	}
	return err;
}

status_t 
BitmapMovie::WriteFrame(BBitmap* bitmap, bool isKeyFrame)
{
	// NULL is not a valid bitmap pointer
	if (!bitmap) return B_BAD_VALUE;

	// if there's no track, this won't work
	if (!mIsOpen) return B_NO_INIT;

	// verify that the bitmap is the right dimensions and that it has the right color space.
	// The "-1" terms are to convert between the BitmapMovie's dimensions, which are
	// numbers of pixels, with the bitmap's dimensions which are an actual *distance*
	// rather than a pixel count.  The distance is one more than the pixel count, hence
	// the adjustment.
	BRect r = bitmap->Bounds();
	if ((r.Width() != mWidth-1) || (r.Height() != mHeight-1) || (bitmap->ColorSpace() != mColorSpace))
	{
		return B_MISMATCHED_VALUES;
	}

	// okay, it's the right kind of bitmap -- commit the header if necessary, and
	// write it as one video frame.  We defer committing the header until the first
	// frame is written in order to allow the client to adjust the image quality at
	// any time up to actually writing video data.
	status_t err = B_OK;
	if (!mHeaderCommitted)
	{
		err = mMediaFile->CommitHeader();
		if (!err) mHeaderCommitted = true;
	}
	if (!err) err = mTrack->WriteFrames(bitmap->Bits(), 1, (isKeyFrame) ? B_MEDIA_KEY_FRAME : 0);
	return err;
}

status_t 
BitmapMovie::CloseFile()
{
	status_t err = B_OK;
	if (mIsOpen && mMediaFile)
	{
		// keep track of whether the file is open so that we don't call
		// CloseFile() twice; this avoids a bug in some current (R4.5.1)
		// file writers.
		mIsOpen = false;
		err = mMediaFile->CloseFile();
	}
	return err;
}

BMediaTrack*
BitmapMovie::GetTrack() const
{
	return mTrack;
}
