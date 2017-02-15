#include "MediaView.h"
#include "MediaSlider.h"
#include "TransportButton.h"
#include "Bitmaps.h"

#include "MediaFile.h"
#include "MediaTrack.h"
#include "AudioOutput.h"

#include <Window.h>
#include <Autolock.h>
#include <Bitmap.h>
#include <ScrollBar.h>
#include <Screen.h>
#include <Path.h>
#include <Entry.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

//#define DEBUG printf
#define DEBUG if (0) printf

const uint32	msg_PlayPause = '_Mpp';

const float		kMediaBarInset 	= 1.0;
const float 	kMediaBarHeight	= kMediaBarInset + kPlayButtonSize.y + kMediaBarInset;


class _MediaBar_ : public BView {
public:
					_MediaBar_(BRect frame, MediaView *owner);

	void			SetTotalTime(bigtime_t totalTime);
	void			SetCurTime(bigtime_t curTime);
	void			ActionUpdate(media_action action);

	virtual void	AttachedToWindow();
	virtual void	Draw(BRect updateRect);
	virtual void	MessageReceived(BMessage *message);

private:
	MediaView*			fOwner;
	PlayPauseButton*	fPlayPauseButton;
	_MediaSlider_*		fMediaSlider;
};


_MediaBar_::_MediaBar_(
	BRect		frame,
	MediaView	*owner)
		: BView(frame, B_EMPTY_STRING, B_FOLLOW_BOTTOM | B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW)
{
	fOwner = owner;

	BRect buttonRect;
	buttonRect.SetLeftTop(BPoint(kMediaBarInset, kMediaBarInset + 1.0));
	buttonRect.SetRightBottom(buttonRect.LeftTop() + kPlayButtonSize);
	
	fPlayPauseButton = new PlayPauseButton(buttonRect,
										   B_EMPTY_STRING,
										   kPlayButtonBitmapBits, 
										   kPressedPlayButtonBitmapBits, 
										   kDisabledPlayButtonBitmapBits,
										   kPlayingPlayButtonBitmapBits, 
										   kPressedPlayingPlayButtonBitmapBits,
										   kPausedPlayButtonBitmapBits, 
										   kPressedPausedPlayButtonBitmapBits,
										   new BMessage(msg_PlayPause), 
										   ' ', 
										   0, 
										   B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	AddChild(fPlayPauseButton);
	fPlayPauseButton->SetPaused();

	BRect sliderRect = Bounds();
	sliderRect.left = buttonRect.right;
	sliderRect.InsetBy(kMediaBarInset * 4, kMediaBarInset);	

	fMediaSlider = new _MediaSlider_(sliderRect, fOwner);
	AddChild(fMediaSlider);

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


void
_MediaBar_::SetTotalTime(
	bigtime_t	totalTime)
{
	fMediaSlider->SetTotalTime(totalTime);
}


void
_MediaBar_::SetCurTime(
	bigtime_t	curTime)
{
	fMediaSlider->SetCurTime(curTime);
}


void
_MediaBar_::ActionUpdate(
	media_action	action)
{
	switch (action) {
		case MEDIA_PLAY:
			fPlayPauseButton->SetPlaying();
			break;

		case MEDIA_STOP:
			fPlayPauseButton->SetPaused();
			break;

		default:
			break;
	}
}


void
_MediaBar_::AttachedToWindow()
{
	fPlayPauseButton->SetTarget(this);
}


void
_MediaBar_::Draw(
	BRect	updateRect)
{
	BRect		bounds = Bounds();
	rgb_color	viewColor = ViewColor();
	rgb_color	light = {255, 255, 255, 255};
	rgb_color	dark = tint_color(viewColor, B_DARKEN_3_TINT);

	BeginLineArray(4);
	AddLine(bounds.RightTop(), bounds.RightBottom(), dark);
	AddLine(bounds.RightBottom(), bounds.LeftBottom(), dark);
	AddLine(bounds.LeftBottom(), bounds.LeftTop(), light);
	AddLine(bounds.LeftTop(), bounds.RightTop(), light);
	EndLineArray();
}


void
_MediaBar_::MessageReceived(
	BMessage	*message)
{
	switch (message->what) {
		case msg_PlayPause:
			fOwner->Control((fOwner->IsPlaying()) ? MEDIA_STOP : MEDIA_PLAY);
			break;

		default:
			break;
	}
}


MediaView::MediaView(
	BRect		frame, 
	const char	*name,
	uint32		resizeMask,
	uint32		flags)
		: BView(frame, name, resizeMask, flags)
{
	InitObject();
}


MediaView::~MediaView()
{
	Stop();
	Reset();
}


status_t
MediaView::SetMediaSource(const char *path)
{
	BAutolock autolock(Window());
	status_t	err = B_ERROR;
	entry_ref	ref;

	err = get_ref_for_path(path, &ref);
	if (err != B_NO_ERROR)
		return (err);

	fMediaFile = new BMediaFile(&ref);
	
	bool	foundTrack = false;
	int32	numTracks = fMediaFile->CountTracks();

	for (int32 i = 0; i < numTracks; i++) {
		BMediaTrack *track = fMediaFile->TrackAt(i);
		
		if (track == NULL) {
			Reset();
			return (B_ERROR);
		}
		else {
			bool			trackUsed = false;
			media_format	mf;

			if (track->EncodedFormat(&mf) == B_NO_ERROR) {			
				switch (mf.type) {
					case B_MEDIA_ENCODED_VIDEO:
printf("#################field rate %f\n", mf.u.encoded_video.output.field_rate);
						trackUsed = SetVideoTrack(path, track, &mf) == B_NO_ERROR;
						break;
	
					case B_MEDIA_RAW_AUDIO:
						trackUsed = SetAudioTrack(path, track, &mf) == B_NO_ERROR;
						break;
						
					case B_MEDIA_ENCODED_AUDIO:
						if (track->DecodedFormat(&mf) == B_NO_ERROR)			
							trackUsed = SetAudioTrack(path, track, &mf) == B_NO_ERROR;
						break;

					default:
						break;
				}
			}
	
			if (trackUsed)
				foundTrack = true;
			else {
				fMediaFile->ReleaseTrack(track);
			}
		}
	}

	if (foundTrack) {
		status_t err = B_ERROR;
	
		fPlayerThread = spawn_thread(MediaView::MediaPlayer, 
									 "MediaView::MediaPlayer",
									 B_NORMAL_PRIORITY,
									 this);
	
		if (fPlayerThread < B_NO_ERROR) {
			err = fPlayerThread;
			fPlayerThread = B_ERROR;
			Reset();
	
			return (err);
		}
	
		fPlaySem = create_sem(0, "MediaView::fPlaySem");
		if (fPlaySem < B_NO_ERROR) {
			err = fPlaySem;
			fPlaySem = B_ERROR;
			Reset();

			return (err);
		}

		err = resume_thread(fPlayerThread);
	
		if (err != B_NO_ERROR) {
			kill_thread(fPlayerThread);
			fPlayerThread = B_ERROR;
			Reset();

			return (err);
		}

		if (fVideoTrack != NULL)
			fMediaBar->SetTotalTime(fVideoTrack->Duration());
		else
			fMediaBar->SetTotalTime(fAudioTrack->Duration());			
	}

	return (B_NO_ERROR);
}


status_t
MediaView::SetColorSpace(
	color_space	depth)
{
	BAutolock autolock(Window());

	fBitmapDepth = depth;

	return (B_NO_ERROR);
}


color_space
MediaView::ColorSpace() const
{
	return (fBitmapDepth);
}


status_t
MediaView::Control(
	media_action	action)
{
	BAutolock autolock(Window());

	status_t err = B_NO_ERROR;

	switch (action) {
		case MEDIA_PLAY:
			err = Play();
			break;

		case MEDIA_STOP:
			err = Stop();
			break;

		default:
			err = B_ERROR;
			break;
	}

	fMediaBar->ActionUpdate(action);

	return (err);
}


bool
MediaView::IsPlaying() const
{
	return (fPlaying);
}


bool
MediaView::HasVideoTrack() const
{
	return (fVideoTrack != NULL);
}


bool
MediaView::HasAudioTrack() const
{
	return (fAudioTrack != NULL);
}


void
MediaView::GetPreferredSize(
	float	*width,
	float	*height)
{
	if (fBitmap == NULL) {
 		BView::GetPreferredSize(width, height);
		*height = kMediaBarHeight;
	}
	else {
		BRect bitmapBounds = fBitmap->Bounds();

		*width = bitmapBounds.Width();
		*height = bitmapBounds.Height() + kMediaBarHeight + 1.0;
	}
}


void
MediaView::Draw(
	BRect	updateRect)
{
	if ((fBitmap != NULL) && !fUsingOverlay)
		DrawBitmap(fBitmap, VideoBounds());
}


void
MediaView::DetachedFromWindow()
{
	Stop();
}


void
MediaView::FrameResized(
	float	width,
	float	height)
{
	Draw(Bounds());
}


void
MediaView::InitObject()
{
	BScreen screen;

	fMediaFile = NULL;
	fVideoTrack = NULL;
	fAudioTrack = NULL;
	fAudioOutput = NULL;
	fMediaBar = NULL;
	fBitmap = NULL;
	fBitmapDepth = screen.ColorSpace();
	fCurTime = 0;
	fPlayerThread = B_ERROR;
	fPlaySem = B_ERROR;
	fScrubSem = B_ERROR;
	fPlaying = false;
	fSnoozing = false;
	fAudioDumpingBuffer = NULL;

	BRect mediaBarFrame = Bounds();
	mediaBarFrame.top = mediaBarFrame.bottom - kMediaBarHeight;
	fMediaBar = new _MediaBar_(mediaBarFrame, this);
	AddChild(fMediaBar);

	SetViewColor(B_TRANSPARENT_32_BIT);
}


status_t
MediaView::SetVideoTrack(
	const char      *path,
	BMediaTrack		*track,
	media_format	*format)
{
	if (fVideoTrack != NULL)
		// is it possible to have multiple video tracks?
		return (B_ERROR);

	fVideoTrack = track;

	BRect bitmapBounds(0.0, 
					   0.0, 
					   format->u.encoded_video.output.display.line_width - 1.0,
					   format->u.encoded_video.output.display.line_count - 1.0);

	fBitmap = new BBitmap(bitmapBounds,B_BITMAP_WILL_OVERLAY|B_BITMAP_RESERVE_OVERLAY_CHANNEL,B_YCbCr422);
	fUsingOverlay = true;
	if (fBitmap->InitCheck() != B_OK) {
		delete fBitmap;
		fBitmap = new BBitmap(bitmapBounds, fBitmapDepth);
		fUsingOverlay = false;
	};

	/* loop, asking the track for a format we can deal with */
	for(;;) {
		media_format mf, old_mf;

		BuildMediaFormat(fBitmap, &mf);

		old_mf = mf;
		fVideoTrack->DecodedFormat(&mf);
		if (old_mf.u.raw_video.display.format == mf.u.raw_video.display.format) {
			break;
		}

		printf("wanted cspace 0x%x, but it was reset to 0x%x\n",
			   fBitmapDepth, mf.u.raw_video.display.format);
		
		fBitmapDepth = mf.u.raw_video.display.format;
		delete fBitmap;
		fUsingOverlay = false;
		fBitmap = new BBitmap(bitmapBounds, fBitmapDepth);
	}

	media_header mh;
	bigtime_t time = fCurTime;	
	fVideoTrack->SeekToTime(&time);

	int64 dummyNumFrames = 0;
	fVideoTrack->ReadFrames((char *)fBitmap->Bits(), &dummyNumFrames, &mh);

	time = fCurTime;
	fVideoTrack->SeekToTime(&time);	

	if (fUsingOverlay) {
		overlay_restrictions r;
		fBitmap->GetOverlayRestrictions(&r);
	
		printf("Overlay limits:\n");
		printf("  Src horiz alignment  : %08x\n",r.source.horizontal_alignment);
		printf("  Src vert alignment   : %08x\n",r.source.vertical_alignment);
		printf("  Src width alignment  : %08x\n",r.source.width_alignment);
		printf("  Src height alignment : %08x\n",r.source.height_alignment);
		printf("  Src min/max          : (%d,%d)/(%d,%d)\n",r.source.min_width,r.source.min_height,
															r.source.max_width,r.source.max_height);
		printf("  Dst horiz alignment  : %08x\n",r.destination.horizontal_alignment);
		printf("  Dst vert alignment   : %08x\n",r.destination.vertical_alignment);
		printf("  Dst width alignment  : %08x\n",r.destination.width_alignment);
		printf("  Dst height alignment : %08x\n",r.destination.height_alignment);
		printf("  Dst min/max          : (%d,%d)/(%d,%d)\n",r.destination.min_width,r.destination.min_height,
															r.destination.max_width,r.destination.max_height);
		printf("  Min/max scaling      : (%f,%f)/(%f,%f)\n",r.min_width_scale,r.min_height_scale,
															r.max_width_scale,r.max_height_scale);
		rgb_color key;
		SetViewOverlay(fBitmap,bitmapBounds,VideoBounds(),&key,B_FOLLOW_ALL,
			B_OVERLAY_FILTER_HORIZONTAL|B_OVERLAY_FILTER_VERTICAL);
		SetViewColor(key);
	};

	return (B_NO_ERROR);
}


status_t
MediaView::SetAudioTrack(
	const char      *path,
	BMediaTrack		*track,
	media_format	*format)
{
	if (fAudioTrack != NULL)
		// is it possible to have multiple tracks?
		return (B_ERROR);

	fAudioTrack = track;

	fAudioOutput = new AudioOutput(fAudioTrack, BPath(path).Leaf());
	status_t err = fAudioOutput->InitCheck();
	if (err != B_NO_ERROR) {
		delete (fAudioOutput);
		fAudioOutput = NULL;
		fAudioTrack = NULL;

		return (err);
	}

	fAudioDumpingBuffer = malloc(format->u.raw_audio.buffer_size);

	return (B_NO_ERROR);
}


status_t
MediaView::Play()
{
	if (fPlaying)
		return (B_NO_ERROR);

	fPlaying = true;
	release_sem(fPlaySem);

	return (B_NO_ERROR);
}


status_t
MediaView::Stop()
{
	if (!fPlaying)
		return (B_NO_ERROR);

	acquire_sem(fPlaySem);
	fPlaying = false;

	return (B_NO_ERROR);
}


void
MediaView::Reset()
{
	delete_sem(fPlaySem);
	fPlaySem = B_ERROR;

	delete_sem(fScrubSem);
	fScrubSem = B_ERROR;

	status_t result = B_NO_ERROR;
	wait_for_thread(fPlayerThread, &result);
	fPlayerThread = B_ERROR;

	fVideoTrack = NULL;

	fAudioTrack = NULL;

	delete (fAudioOutput);
	fAudioOutput = NULL;

	delete (fBitmap);
	fBitmap = NULL;

	delete (fMediaFile);
	fMediaFile = NULL;

	free(fAudioDumpingBuffer);
	fAudioDumpingBuffer = NULL;
}


BRect
MediaView::VideoBounds() const
{
	BRect videoBounds = Bounds();
	videoBounds.bottom -= kMediaBarHeight + 1;

	return (videoBounds);
}


void
MediaView::BuildMediaFormat(
	BBitmap			*bitmap,
	media_format	*format)
{
	media_raw_video_format *rvf = &format->u.raw_video;

	memset(format, 0, sizeof(*format));

	BRect bitmapBounds = bitmap->Bounds();

	rvf->last_active = (uint32)(bitmapBounds.Height() - 1.0);
	rvf->orientation = B_VIDEO_TOP_LEFT_RIGHT;
	rvf->pixel_width_aspect = 1;
	rvf->pixel_height_aspect = 3;
	rvf->display.format = bitmap->ColorSpace();
	rvf->display.line_width = (int32)bitmapBounds.Width();
	rvf->display.line_count = (int32)bitmapBounds.Height();
	rvf->display.bytes_per_row = bitmap->BytesPerRow();
}

int32
MediaView::MediaPlayer(
	void	*arg)
{
	MediaView*		view = (MediaView *)arg;
	BWindow*		window = view->Window();
	BBitmap*		bitmap = view->fBitmap;
	BMediaTrack*	videoTrack = view->fVideoTrack;
	BMediaTrack*	audioTrack = view->fAudioTrack;
	BMediaTrack*	counterTrack = (videoTrack != NULL) ? videoTrack : audioTrack;
	AudioOutput*	audioOutput = view->fAudioOutput;
	void*			adBuffer = view->fAudioDumpingBuffer;
	bigtime_t		totalTime = counterTrack->Duration();	
	int64			numFrames = counterTrack->CountFrames();
	int64			numFramesToSkip = 0;
	int64			numSkippedFrames = 0;
	bool			scrubbing = false;
	bool			seekNeeded = false;
	int64			dummy;
	media_header	mh;
	bigtime_t		vStartTime, aStartTime, seekTime, snoozeTime, startTime;
	bigtime_t		curScrubbing, lastScrubbing, lastTime;

	curScrubbing = lastScrubbing = system_time();
	seekTime = 0LL;

	// Main processing loop (handle stop->start->stop)
	while (acquire_sem(view->fPlaySem) == B_OK) {
		release_sem(view->fPlaySem);
		
		// as we are doing stop->start, restart audio if needed.
		if (audioTrack != NULL)
			audioOutput->Play();
		startTime = system_time()-counterTrack->CurrentTime();		

		// This will loop until the end of the stream
		while ((counterTrack->CurrentFrame() < numFrames) || scrubbing) {
		
			// We are in scrub mode
			if (acquire_sem(view->fScrubSem) == B_OK) {
				curScrubbing = system_time();

				// We are entering scrub mode
				if (!scrubbing) {
					if (audioTrack != NULL)
						audioOutput->Stop();
					scrubbing = true;
				}
				// Do a seek.
				seekNeeded = true;
				seekTime = view->fScrubTime;
			}
			// We are not scrubbing
			else if (scrubbing) {
				if (audioTrack != NULL)
					audioOutput->Play();
				scrubbing = false;
			}
			
			// Handle seeking
			if (seekNeeded) {
				if (videoTrack) {
					// Seek the seekTime as close as possible
					vStartTime = seekTime;
					videoTrack->SeekToTime(&vStartTime);
					
					// Read frames until we get less than 50ms ahead.
					lastTime = vStartTime;
					do {
						bitmap->LockBits();
						status_t err = videoTrack->ReadFrames((char*)bitmap->Bits(), &dummy, &mh);
						bitmap->UnlockBits();
						if (err != B_OK) break;
						vStartTime = mh.start_time;
						if ((dummy == 0) || (vStartTime <= lastTime))
							break;
						lastTime = vStartTime;
					} while (seekTime - vStartTime > 50000);
				}
				
				if (audioTrack) {
					// Seek the extractor as close as possible
					aStartTime = seekTime;
					audioOutput->SeekToTime(&aStartTime);
					
					// Read frames until we get less than 50ms ahead.
					lastTime = aStartTime;
					while (seekTime - aStartTime > 50000) {
						if (audioTrack->ReadFrames((char *)adBuffer, &dummy, &mh) != B_OK)
							break;
						aStartTime = mh.start_time;
						if ((dummy == 0) || (aStartTime <= lastTime))
							break;
						lastTime = aStartTime;
					}
				}
				else startTime = system_time() - vStartTime;
				
				// Set the current time
				view->fCurTime = seekTime;	
			
				seekNeeded = false;
			}		
			// Handle normal playing mode
			else {
				// Get the next video frame, if any
				if (videoTrack != NULL) {
					bitmap->LockBits();
					status_t err = videoTrack->ReadFrames((char*)bitmap->Bits(), &dummy, &mh);
					bitmap->UnlockBits();
					if (err != B_OK) goto do_reset;
					if (dummy == 0)
						goto do_reset;
					vStartTime = mh.start_time;
				}

				// Estimated snoozeTime
				if (audioTrack != NULL)
					startTime = audioOutput->TrackTimebase();
				if (videoTrack != NULL)
					snoozeTime = vStartTime - (system_time() - startTime);
				else
					snoozeTime = 25000;

				// Handle timing issues
				if (snoozeTime > 5000LL) {
					view->fSnoozing = true;
					snooze(snoozeTime-1000);
					view->fSnoozing = false;
				}
				else if (snoozeTime < -5000) {
					numSkippedFrames++;
					numFramesToSkip++;
				}
				
				// Set the current time
				if (!scrubbing) {
					view->fCurTime = system_time() - startTime;
					if (view->fCurTime < seekTime)
						view->fCurTime = seekTime;
				}				
			}
				
			// Handle the drawing : no drawing if we need to skip a frame...
			if (numSkippedFrames > 0)
				numSkippedFrames--;
			// If we can't lock the window after 50ms, better to give up for
			// that frame...
			else if (window->LockWithTimeout(50000) == B_OK) {
				if ((videoTrack != NULL) && !view->fUsingOverlay)
					view->DrawBitmap(bitmap, view->VideoBounds());
				view->fMediaBar->SetCurTime(view->fCurTime);
				window->Unlock();
				// In scrub mode, don't scrub more than 10 times a second
				if (scrubbing) {
					snoozeTime = (100000LL+lastScrubbing) - system_time();
					if (snoozeTime > 4000LL) {
						view->fSnoozing = true;
						snooze(snoozeTime-1000LL);
						view->fSnoozing = false;
					}
					lastScrubbing = curScrubbing;
				}
			}				
			
			// Check if we are required to stop.
			if (acquire_sem_etc(view->fPlaySem, 1, B_TIMEOUT, 0) == B_OK)
				release_sem(view->fPlaySem);
			// The MediaView asked us to stop.
			else {
				if (audioTrack != NULL)
					audioOutput->Stop();
				goto do_restart;
			}
DEBUG("############ Current frame:%Ld, total frame:%Ld\n", counterTrack->CurrentFrame(), numFrames);
		}		

		// If we exited the main streaming loop because we are at the end,
		// then we need to loop.
		if (counterTrack->CurrentFrame() >= numFrames) {
do_reset:
			if (audioTrack != NULL)
				audioOutput->Stop();
				
			seekNeeded = true;
			seekTime = 0LL;
			scrubbing = true;
		}
do_restart:;
	}

	return (B_NO_ERROR);
}
