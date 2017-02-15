/*******************************************************************************
/
/	File:			utilmedia.cpp
/
/   Description:	Useful helper functions when working with the Media Kit.
/
*******************************************************************************/
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Debug.h>
#include <scheduler.h>
#include "utilmedia.h"

static void specialize_raw_audio_format(media_raw_audio_format* fmt, const media_raw_audio_format& model);
static void specialize_raw_video_format(media_raw_video_format* fmt, const media_raw_video_format& model);
static void specialize_video_display_info(media_video_display_info* info, const media_video_display_info& model);

void specialize_format(media_format* format, const media_format& model)
{
	ASSERT(format && format->type == model.type);
	switch (format->type) {
	case B_MEDIA_RAW_AUDIO:
		{
			media_raw_audio_format& fmt = format->u.raw_audio;
			specialize_raw_audio_format(&fmt, model.u.raw_audio);
			break;
		}
	case B_MEDIA_RAW_VIDEO:
		{
			media_raw_video_format& fmt = format->u.raw_video;
			specialize_raw_video_format(&fmt, model.u.raw_video);
			break;
		}
	default:
		// TODO: implement other cases if necessary
		DEBUGGER("specialize_format can't handle this format yet!");
		break;
	}
}

void specialize_raw_audio_format(media_raw_audio_format* fmt, const media_raw_audio_format& model)
{
	media_raw_audio_format& wc = media_raw_audio_format::wildcard;

	if (fmt->frame_rate == wc.frame_rate)
		fmt->frame_rate = model.frame_rate;		
	if (fmt->channel_count == wc.channel_count)
		fmt->channel_count = model.channel_count;		
	if (fmt->format == wc.format)
		fmt->format = model.format;
	if (fmt->byte_order == wc.byte_order)
		fmt->byte_order = model.byte_order;		
	if (fmt->buffer_size == wc.buffer_size)
		fmt->buffer_size = model.buffer_size;
}

void specialize_raw_video_format(media_raw_video_format* fmt, const media_raw_video_format& model)
{
	media_raw_video_format& wc = media_raw_video_format::wildcard;
	
	if (fmt->field_rate == wc.field_rate)
		fmt->field_rate = model.field_rate;		
	if (fmt->interlace == wc.interlace)
		fmt->interlace = model.interlace;		
	if (fmt->first_active == wc.first_active)
		fmt->first_active = model.first_active;		
	if (fmt->last_active == wc.last_active)
		fmt->last_active = model.last_active;		
	if (fmt->orientation == wc.orientation)
		fmt->orientation = model.orientation;		
	if (fmt->pixel_width_aspect == wc.pixel_width_aspect)
		fmt->pixel_width_aspect = model.pixel_width_aspect;		
	if (fmt->pixel_height_aspect == wc.pixel_height_aspect)
		fmt->pixel_height_aspect = model.pixel_height_aspect;		

	media_video_display_info& info = fmt->display;
	specialize_video_display_info(&info, model.display);
}

void specialize_video_display_info(media_video_display_info* info, const media_video_display_info& model)
{
	media_video_display_info& wc = media_video_display_info::wildcard;
	
	if (info->format == wc.format)
		info->format = model.format;
	if (info->line_width == wc.line_width)
		info->line_width = model.line_width;
	if (info->line_count == wc.line_count)
		info->line_count = model.line_count;
	if (info->bytes_per_row == wc.bytes_per_row)
		info->bytes_per_row = model.bytes_per_row;
	if (info->pixel_offset == wc.pixel_offset)
		info->pixel_offset = model.pixel_offset;
	if (info->line_offset == wc.line_offset)
		info->line_offset = model.line_offset;	
}

void wildcard_format(media_format* format)
{
	ASSERT(format);
	switch (format->type) {
	case B_MEDIA_RAW_AUDIO:
		format->u.raw_audio = media_raw_audio_format::wildcard;
		break;
	case B_MEDIA_RAW_VIDEO:
		format->u.raw_video = media_raw_video_format::wildcard;
		break;
	case B_MEDIA_MULTISTREAM:
		format->u.multistream = media_multistream_format::wildcard;
		break;
	case B_MEDIA_ENCODED_AUDIO:
		format->u.encoded_audio = media_encoded_audio_format::wildcard;
		break;
	case B_MEDIA_ENCODED_VIDEO:
		format->u.encoded_video = media_encoded_video_format::wildcard;
		break;
	default:
		// no wildcard needed
		break;
	}
}

int32 suggest_priority_from_format(const media_format& format)
{
	int32 priority = B_NORMAL_PRIORITY;
	switch (format.type) {
	case B_MEDIA_RAW_AUDIO:
		{
			bigtime_t dur = buffer_duration(format.u.raw_audio);
			priority = suggest_thread_priority(B_LIVE_AUDIO_MANIPULATION, dur, dur/2);
			break;
		}	
	case B_MEDIA_RAW_VIDEO:
		{
			bigtime_t dur = (bigtime_t) (1000000.0 / format.u.raw_video.field_rate);
			priority = suggest_thread_priority(B_LIVE_VIDEO_MANIPULATION, dur, dur/2);
			break;
		}
	default:
		DEBUGGER("suggest_priority_from_format can't handle this format yet!");
		break;
	}
	return priority;
}

bigtime_t buffer_duration(const media_format& format)
{
	switch (format.type) {
	case B_MEDIA_RAW_AUDIO:
		return buffer_duration(format.u.raw_audio);
	case B_MEDIA_RAW_VIDEO:
		return buffer_duration(format.u.raw_video);
	default:
		DEBUGGER("buffer_duration can't handle this format yet!");
		return 0;
	}	
}

// These two conversions seem to pop up all the time in media code.
// I guess it's the curse of microsecond resolution... ;-)
double
us_to_s(bigtime_t usecs)
{
	return (usecs / 1000000.0);
}

bigtime_t
s_to_us(double secs)
{
	return (bigtime_t) (secs * 1000000.0);
}

int
bytes_per_frame(
	const media_raw_audio_format & format)
{
	//	The media_raw_audio_format format constants encode the
	//	bytes-per-sample value in the low nybble. Having a fixed
	//	number of bytes-per-sample, and no inter-sample relationships,
	//	is what makes a format "raw".
	int bytesPerSample = format.format & 0xf;
	return bytesPerSample * format.channel_count;
}

int
frames_per_buffer(
	const media_raw_audio_format & format)
{
	// This will give us the number of full-sized frames that will fit
	// in a buffer. (Remember, integer division automatically rounds
	// down.)
	int frames = 0;
	if (bytes_per_frame(format) > 0) {
		frames = format.buffer_size / bytes_per_frame(format);
	}
	return frames;
}

bigtime_t
buffer_duration(
	const media_raw_audio_format & format)
{
	//	Figuring out duration is easy. We take extra precaution to
	//	not divide by zero or return irrelevant results.
	bigtime_t duration = 0;
	if (format.buffer_size > 0 && format.frame_rate > 0 && bytes_per_frame(format) > 0) {
		//	In these kinds of calculations, it's always useful to double-check
		//	the unit conversions. (Anyone remember high school physics?)
		//	bytes/(bytes/frame) / frames/sec
		//	= frames * sec/frames
		//	= secs                            which is what we want.
		duration = s_to_us((format.buffer_size / bytes_per_frame(format)) / format.frame_rate);
	}
	return duration;
}

bigtime_t
frames_duration(
	const media_raw_audio_format & format, int64 num_frames)
{
	//	Tells us how long in us it will take to produce num_frames,
	//	with the given format.
	bigtime_t duration = 0;
	if (format.frame_rate > 0) {
		duration = s_to_us(num_frames/format.frame_rate);
	}
	return duration;
}

int
buffers_for_duration(
	const media_raw_audio_format & format, bigtime_t duration)
{
	// Double-checking those unit conversions again:
	// secs * ( (frames/sec) / (frames/buffer) ) = secs * (buffers/sec) = buffers
	int buffers = 0;
	if (frames_per_buffer(format) > 0) {
		buffers = (int) ceil(us_to_s(duration)*(format.frame_rate/frames_per_buffer(format)));
	}
	return buffers;
}

int64
frames_for_duration(
	const media_raw_audio_format & format, bigtime_t duration)
{
	return (int64) ceil(format.frame_rate*us_to_s(duration));
}

bigtime_t 
buffer_duration(const media_raw_video_format& fmt)
{
	return s_to_us(1/fmt.field_rate);
}
