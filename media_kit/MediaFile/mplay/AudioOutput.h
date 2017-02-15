#ifndef _AUDIO_OUTPUT_H
#define _AUDIO_OUTPUT_H

#include <SoundPlayer.h>
#include <OS.h>
#include "MediaTrack.h"

class AudioOutput {
public :
					AudioOutput(BMediaTrack *track, const char *name);
					~AudioOutput();
	status_t		InitCheck() { return ((player != NULL)?B_OK:B_ERROR); };
	BMediaTrack		*Track() { return track; };
	status_t		SeekToTime(bigtime_t *inout_time);
	status_t		Play();
	status_t		Stop();
	bool			IsPlaying() { return isPlaying; };
	bigtime_t		TrackTimebase();

private :
friend void AudioPlay(void *, void *, size_t, const media_raw_audio_format &);

	void			Lock();
	void			Unlock();

	bool			isPlaying;
	int32			frameSize;
	int32			lock_count;
	int32			channelCount;
	int8			default_data;
	uint32			frame_size;
	float			frame_rate;
	uint32			buffer_size;
	sem_id			lock_sem;
	BMediaTrack		*track;
	BSoundPlayer	*player;
	bigtime_t		perfTime, trackTime;
};

#endif
