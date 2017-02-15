#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <Application.h>
#include <SupportDefs.h>
#include <Bitmap.h>
#include <Rect.h>
#include <MediaDefs.h>
#include <Entry.h>

#include "MediaFile.h"
#include "MediaTrack.h"

// this small sample code shows how to extract video frames and audio
// samples from a file.


// ProcessBitmap() is called on every decoded video frame. Useful code could be
// added here.

void
ProcessBitmap(BBitmap *bitmap)
{
	printf("Processing a video frame...\n");
}

// ProcessAudio() is called on every chunk of audio. Useful code could be added
// here.

void
ProcessAudio(char *buffer, media_format *format, int64 sampleNum)
{
	printf("Processing an audio frame\n");
}


int
main(int argc, char **argv)
{
	status_t		err;
	entry_ref		ref;
	media_format	format;
	BMediaFile		*mediaFile;
	BMediaTrack		*track;
	int32			numTracks;
	int64			numFrames;
	int32			i;
	int64			j;
	int64			frameCount;
	bigtime_t		dummyTime;
	BRect			bounds;
	BBitmap			*bitmap;
	char			*buffer;
	media_header	mh;

	BApplication	app("application/x-vnd.Be-cmd-DMMY");

	if (argc != 2) {
		printf("usage: %s <filename>\n", argv[0]);
		return 1;
	}

	// get the ref from the path

	err = get_ref_for_path(argv[1], &ref);
	if (err) {
		printf("problem with get_ref_for_path() -- %s\n", strerror(err));
		return 1;
	}

	// instantiate a BMediaFile object, and make sure there was no error.
	mediaFile = new BMediaFile(&ref);
	err = mediaFile->InitCheck();
	if (err) {
		printf("cannot contruct BMediaFile object -- %s\n", strerror(err));
		return 1;
	}
	
	// count the tracks and instanciate them, one at a time

	numTracks = mediaFile->CountTracks();

	for (i = 0; i < numTracks; i++) {
		track = mediaFile->TrackAt(i);
		if (!track) {
			printf("cannot contruct BMediaTrack object\n");
			return 1;
		}

		// get the encoded format

		err = track->EncodedFormat(&format);
		if (err) {
			printf("BMediaTrack::EncodedFormat error -- %s\n", strerror(err));
			return 1;
		}

		switch(format.type) {

		// handle the case of an encoded video track

	 	case B_MEDIA_ENCODED_VIDEO:

			// allocate a bitmap large enough to contain the decoded frame.

			bounds.Set(0.0, 0.0, format.u.encoded_video.output.display.line_width - 1.0, 
					   format.u.encoded_video.output.display.line_count - 1.0);
			bitmap = new BBitmap(bounds, B_RGB32);

			// specifiy the decoded format. we derive this information from
			// the encoded format.

			memset(&format, 0, sizeof(media_format));
			format.u.raw_video.last_active = (int32) (bounds.Height() - 1.0);
			format.u.raw_video.orientation = B_VIDEO_TOP_LEFT_RIGHT;
			format.u.raw_video.pixel_width_aspect = 1;
			format.u.raw_video.pixel_height_aspect = 3;
			format.u.raw_video.display.format = bitmap->ColorSpace();
			format.u.raw_video.display.line_width = (int32) bounds.Width();
			format.u.raw_video.display.line_count = (int32) bounds.Height();
			format.u.raw_video.display.bytes_per_row = bitmap->BytesPerRow();
			err = track->DecodedFormat(&format);
			if (err) {
				printf("error with BMediaTrack::DecodedFormat() -- %s\n", strerror(err));
				return 1;
			}

			// iterate through all the frames and call the processing function

			buffer = (char *)bitmap->Bits();
			numFrames = track->CountFrames();

			for(j = 0; j < numFrames; j++) {
				err = track->ReadFrames(buffer, &frameCount, &mh);
				if (err) {
					printf("BMediaTrack::ReadFrames error -- %s\n", strerror(err));
					return 1;
				}
				ProcessBitmap(bitmap);			
			}
			delete bitmap;
			break;

		// handle the case of a raw audio track

		case B_MEDIA_RAW_AUDIO:

			// no need to call DecodedFormat, as this is an already decoded track

			// iterate through all the audio chunks and call the processing function

			buffer = (char *)malloc(format.u.raw_audio.buffer_size);
			numFrames = track->CountFrames();

			for(j = 0; j < numFrames; j+=frameCount) {
				err = track->ReadFrames(buffer, &frameCount, &mh);
				if (err) {
					printf("BMediaTrack::ReadFrames error -- %s\n", strerror(err));
					return 1;
				}
				ProcessAudio(buffer, &format, frameCount);			
			}
			free(buffer);
			break;
		}

		mediaFile->ReleaseTrack(track);
	}
	return 0;
}
