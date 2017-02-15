#include <storage/Node.h>
#include <storage/NodeInfo.h>
#include <String.h>
#include <stdio.h>
#include "FilmStrip.h"
#include "ErrorAlert.h"

FilmStrip::FilmStrip(const entry_ref& ref)
	: mStatus(B_NO_INIT), mFile(NULL), mTrack(NULL), mFrame(NULL),
		mMode(kFrame)
{	
	mStatus = SetTo(ref);
	if (mStatus == B_OK) {
		// attempt at intelligent (ha!) parameter setting:
		// set the grab interval such that we get at least
		// five frames from the movie (including the first),
		// but we prefer an interval of 2 sec. if possible
		bigtime_t duration = mTrack->Duration();
		mFrameInterval = duration >= 8000000 ? 2000000
			: duration/4;
	}
}

FilmStrip::FilmStrip(const entry_ref& ref,const int32 mode,
		const bigtime_t grabInterval)
	: mStatus(B_NO_INIT), mFile(NULL), mTrack(NULL), mFrame(NULL),
		mFrameInterval(grabInterval), mMode(mode)
{	
	mStatus = SetTo(ref);
}

FilmStrip::~FilmStrip(void)
{
	delete mFile; // will delete mTrack as well
	delete mFrame;
}

status_t FilmStrip::SetTo(const entry_ref& ref)
{
	status_t err;

	err = SniffRef(ref);
	if (err != B_OK) {
		// no need to continue
		return err;
	}

	mFile = new BMediaFile(&ref);
	err = mFile->InitCheck();
	if (err == B_OK) {
		int32 i = 0;
		do {
			mTrack = mFile->TrackAt(i);
			err = mTrack->InitCheck();
			if (err == B_OK) {
				// sniff out whether it's the video track
				// and break out if it is
				mFormat.u.raw_video = media_raw_video_format::wildcard;
				mFormat.type = B_MEDIA_RAW_VIDEO;
				err = mTrack->DecodedFormat(&mFormat);
				if (err == B_OK) {
					if (mFormat.IsVideo()) {
						break;
					}
					else {
						// when mFile is deleted it will
						// delete all open tracks as well,
						// but why waste the memory
						// on tracks we're not using?
						mFile->ReleaseTrack(mTrack);
					}
				}
			}
			i++;
		} while (i < mFile->CountTracks());

		if (err == B_OK) {
			printf("length of movie: %Ld\n",mTrack->Duration());
		}
	}
	
	return err;
}

BBitmap* FilmStrip::NextFrame(void)
{
	status_t err;
	bigtime_t newTime = 0;
	int64 frame = 0;
	int64 numFrames;
	
	// create mFrame if we haven't already
	if (mFrame == NULL) {
		BRect bounds(0,0,mFormat.Width()-1,mFormat.Height()-1);
		mFrame = new BBitmap(bounds,mFormat.ColorSpace());
	}
	// stuff the bits from the desired frame in mFrame, then
	// seek ahead for the next frame grab
	if (mFrame != NULL && mFrame->IsValid()) {
		printf("time: %Ld\n",mTrack->CurrentTime());
		printf("frame: %Ld\n",mTrack->CurrentFrame());
		err = mTrack->ReadFrames((char*)mFrame->Bits(),&numFrames,
			NULL);
		// we don't really need separate code paths for kTime
		// and kFrame below -- we can convert between number of
		// frames and time -- but this shows explicitly the two
		// ways of working with BMediaTrack
		switch (mMode) {
		case kTime:
			newTime = mTrack->CurrentTime() + mFrameInterval;
			if (newTime > mTrack->Duration()) {
				newTime = 0;
			}
			mTrack->SeekToTime(&newTime,B_MEDIA_SEEK_CLOSEST_FORWARD);
			break;
		case kFrame:
			frame = mTrack->CurrentFrame() + 1;
			if (frame > mTrack->CountFrames()) {
				frame = 0;
			}
			mTrack->SeekToFrame(&frame,B_MEDIA_SEEK_CLOSEST_FORWARD);
			break;
		}
	}

	return mFrame;
}

status_t FilmStrip::SniffRef(const entry_ref& ref)
{
	status_t err;
	char type[256];

	// quick check that ref is some kind of movie
	BNode node(&ref);
	if ((err = node.InitCheck()) == B_OK) {
		BNodeInfo nodeInfo(&node);
		if (nodeInfo.InitCheck() == B_OK) {
			err = nodeInfo.GetType(type);
			if (err == B_OK) {
				BString mask("video/");
				if (mask.Compare(type,mask.CountChars()) != 0) {
					err = NOT_A_MOVIE;
				}
			}
		}
	}	
	
	return err;
}

const bigtime_t FilmStrip::Duration(void) const
{
	return mTrack->Duration();
}

const bigtime_t& FilmStrip::FrameInterval(void) const
{
	return mFrameInterval;
}

void FilmStrip::SetFrameInterval(const bigtime_t& interval)
{
	mFrameInterval = interval;
}

const media_format& FilmStrip::Format(void) const
{
	return mFormat;
}

const int32 FilmStrip::Mode(void) const
{
	return mMode;
}

void FilmStrip::SetMode(const int32 mode)
{
	mMode = mode;
}