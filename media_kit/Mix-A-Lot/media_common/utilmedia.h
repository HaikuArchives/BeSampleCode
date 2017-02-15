/*******************************************************************************
/
/	File:			utilmedia.h
/
/   Description:	Useful helper functions when working with the Media Kit.
/
*******************************************************************************/
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _utilmedia_h
#define _utilmedia_h

#include <MediaDefs.h>
#include <ByteOrder.h>

#if GENKI_4
// A temporary message protocol for latency changes in genki/4.
// This is similar, but not identical, to the implementation for
// genki/5.
const int32 MY_LATENCY_CHANGED = 0x64000000;
struct my_latency_change_request {
	media_source src;
	media_destination dest;
	bigtime_t latency;
	uint32 flags;
};
#endif

// Generally, all you care about is whether the data
// is host-endian or not!
#if B_HOST_IS_LENDIAN
	#define	MEDIA_HOST_ENDIAN	B_MEDIA_LITTLE_ENDIAN
#elif B_HOST_IS_BENDIAN
	#define	MEDIA_HOST_ENDIAN	B_MEDIA_BIG_ENDIAN
#else
	#error "The host byte order must be defined."
#endif

// Specialize all wildcards in the target format by filling
// them in with the specified fields from the model.
void specialize_format(media_format* format, const media_format& model);

// Given a format with a media type, this will fill in all
// of the format type-specific fields with wildcards.
void wildcard_format(media_format* format);

// Tells you what priority your thread should run at if it needs
// to process data in the given format.
int32 suggest_priority_from_format(const media_format& format);

// Calculate the duration of one buffer, given a raw audio/video format.
bigtime_t buffer_duration(const media_format & format);
bigtime_t buffer_duration(const media_raw_audio_format& fmt);
bigtime_t buffer_duration(const media_raw_video_format& fmt);

//	Microsecond <-> second conversion
double us_to_s(bigtime_t usecs);
bigtime_t s_to_us(double secs);

// The following functions apply to raw audio formats only!
int bytes_per_frame(const media_raw_audio_format & format);
int frames_per_buffer(const media_raw_audio_format & format);
bigtime_t frames_duration(const media_raw_audio_format & format,
	int64 num_frames);
int64 frames_for_duration(const media_raw_audio_format & format,
	bigtime_t duration);
int buffers_for_duration(const media_raw_audio_format & format,
	bigtime_t duration);

#endif /* _utilmedia_h */