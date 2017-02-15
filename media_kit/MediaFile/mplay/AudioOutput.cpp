#include "AudioOutput.h"
#include <stdio.h>
#include <string.h>

void AudioPlay(void *cookie, void *buffer, size_t bufferSize, const media_raw_audio_format &format)
{
	bool			update_trackTime;
	int64			frame_count;
	uint32			i, filled;
	status_t		err;
	AudioOutput		*ao;
	media_header    mh;

	ao = (AudioOutput*)cookie;
	ao->Lock();

	update_trackTime = true;

	if (ao->isPlaying) {
		err = ao->track->ReadFrames((char*)buffer, &frame_count, &mh);
		if ((err != B_OK) || (frame_count < 0)) {
			memset((char*)buffer, ao->default_data, ao->buffer_size);
			update_trackTime = false;
		}
		else {
			filled = ao->frame_size * frame_count;
			if (filled < ao->buffer_size)
				memset((char*)buffer+filled, ao->default_data, ao->buffer_size-filled);
			if (err != B_OK)
				update_trackTime = false;
		}
	}
	else
		memset((char*)buffer, ao->default_data, ao->buffer_size);

	ao->perfTime = ao->player->PerformanceTime();
	if (update_trackTime)
		ao->trackTime = ao->track->CurrentTime();
	else
		ao->trackTime +=
			(bigtime_t)(1e6*(float)bufferSize/((float)ao->frame_size*ao->frame_rate));

	ao->Unlock();
}

AudioOutput::AudioOutput(BMediaTrack *new_track, const char *name) {
	media_format	format;

	lock_count = 0;
	lock_sem = create_sem(0, "audio_output ben");
	track = new_track;
	isPlaying = false;
	perfTime = -1;
	perfTime = -1;
	trackTime = 0;
	
	track->DecodedFormat(&format);
	switch (format.u.raw_audio.format) {
	case media_raw_audio_format::B_AUDIO_UCHAR :
		default_data = 0x80;
		frame_size = 1;
		break;
	case media_raw_audio_format::B_AUDIO_SHORT :
		default_data = 0;
		frame_size = 2;
		break;
	case media_raw_audio_format::B_AUDIO_INT :
		default_data = 0;
		frame_size = 4;
		break;
	case media_raw_audio_format::B_AUDIO_FLOAT :
		default_data = 0;
		frame_size = 4;
		break;
	default :
		player = NULL;
		return;
	}
	channelCount = format.u.raw_audio.channel_count;
	frame_size *= channelCount;
	buffer_size = format.u.raw_audio.buffer_size;
	frame_rate = format.u.raw_audio.frame_rate;

	player = new BSoundPlayer(&format.u.raw_audio, name, AudioPlay);
	if (player->InitCheck() != B_OK) {
		delete player;
		player = NULL;
	} else {
		player->SetCookie(this);
		player->Start();
		player->SetHasData(true);
	}
}

AudioOutput::~AudioOutput() {
	if (player)
		player->Stop();
	delete player;
	delete_sem(lock_sem);
}

void AudioOutput::Lock() {
	if (atomic_add(&lock_count, 1) > 0)
		acquire_sem(lock_sem);
}

void AudioOutput::Unlock() {
	if (atomic_add(&lock_count, -1) > 1)
		release_sem(lock_sem);
}

status_t AudioOutput::SeekToTime(bigtime_t *inout_time) {
	status_t	err;

	Lock();
	err = track->SeekToTime(inout_time);
	trackTime = *inout_time;
	Unlock();
	return err;
}

status_t AudioOutput::Play() {
	Lock();
	isPlaying = true;
	Unlock();
	return B_NO_ERROR;
}

status_t AudioOutput::Stop() {
	isPlaying = false;
	return B_NO_ERROR;
}

bigtime_t AudioOutput::TrackTimebase() {
	return perfTime - trackTime;
}
