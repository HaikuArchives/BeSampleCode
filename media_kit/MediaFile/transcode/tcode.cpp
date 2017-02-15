#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#include <Application.h>
#include <SupportDefs.h>
#include <Bitmap.h>
#include <Rect.h>
#include <MediaDefs.h>
#include <MediaFormats.h>
#include <Screen.h>
#include <GraphicsDefs.h>
#include <Entry.h>

#include "MediaFile.h"
#include "MediaTrack.h"


void
BuildMediaFormat(int32 width, int32 height, color_space cspace,
				 media_format *format)
{
	media_raw_video_format *rvf = &format->u.raw_video;

	memset(format, 0, sizeof(*format));

	format->type = B_MEDIA_RAW_VIDEO;
	rvf->last_active = (uint32)(height - 1);
	rvf->orientation = B_VIDEO_TOP_LEFT_RIGHT;
	rvf->pixel_width_aspect = 1;
	rvf->pixel_height_aspect = 3;
	rvf->display.format = cspace;
	rvf->display.line_width = (int32)width;
	rvf->display.line_count = (int32)height;
	if (cspace == B_RGB32)
		rvf->display.bytes_per_row = 4 * width;
	else {
		printf("goodbye arve\n");
		exit(5);
	}
}


void
transcode(BMediaTrack *vidtrack, BMediaTrack *audtrack,
		  char *output, char *family_name, char *video_name, char *audio_name)
{
	char					*chunk;
	char					*bitmap = NULL, *sound_buffer = NULL;
	bool            		found_video_encoder = false, found_audio_encoder = false;
	bool					found_family;
	int32					i, sz, cookie;
	int64					numFrames, j;
	int64					framesize;
	status_t				err;
	entry_ref       		ref;
	BMediaFile      		*out;
	BMediaTrack    		 	*vid = NULL, *aud = NULL;
	media_format			format, outfmt;
	media_codec_info    	mci;
	media_file_format		mfi;
	media_header 			mh;

	// get the ref from the path
	err = get_ref_for_path(output, &ref);
	if (err) {
		printf("problem with get_ref_for_path() -- %s\n", strerror(err));
		return;
	}

	// find the right file -format handler
	cookie = 0;
	while((err = get_next_file_format(&cookie, &mfi)) == B_OK) {
		if (strcmp(mfi.short_name, family_name) == 0)
			break;
	}

	if (err != B_OK) {
		printf("failed to find a file format handler !\n");
		return;
	}

	// Create and init the mediaFile
	out = new BMediaFile(&ref, &mfi);
	err = out->InitCheck();
	if (err != B_OK) {
		printf("failed to properly init the output file... (%s)\n", strerror(err));
		delete out;
		return;
	}

	// Create the video track if any	
	if (vidtrack) {
		vidtrack->EncodedFormat(&format);
		
		if (video_name) {
			int width, height;
			width = format.u.encoded_video.output.display.line_width;
			height = format.u.encoded_video.output.display.line_count;
	
			memset(&format, 0, sizeof(format));
			BuildMediaFormat(width, height, B_RGB32, &format);
	
			vidtrack->DecodedFormat(&format);
			
			bitmap = (char *)malloc(width * height * 4);
	
			cookie = 0;
			while (get_next_encoder(&cookie, &mfi, &format, &outfmt, &mci) == B_OK) {
				printf("found encoder %s (%d)\n", mci.pretty_name, mci.id);
				if (strcmp(video_name, mci.short_name) == 0) {
					found_video_encoder = true;
					break;
				}
			}
		}
	
		if (found_video_encoder) 
			vid = out->CreateTrack(&format, &mci);
		else
			vid = out->CreateTrack(&format);
			
		if (vid == NULL) {
			printf("Failed to create video track\n");
			delete out;
			return;
		}
	}

	// Create the audio track if any
	if (audtrack) {
		audtrack->EncodedFormat(&format);

		audtrack->DecodedFormat(&format);
		sound_buffer = (char*)malloc(format.u.raw_audio.buffer_size);
		framesize = (format.u.raw_audio.format&15)*format.u.raw_audio.channel_count;
	
		if (audio_name) {
			cookie = 0;
			while (get_next_encoder(&cookie, &mfi, &format, &outfmt, &mci) == B_OK) {
				printf("found encoder %s (%d)\n", mci.pretty_name, mci.id);
				if (strcmp(audio_name, mci.short_name) == 0) {
					found_audio_encoder = true;
					break;
				}
			}
		}
	
		if (found_audio_encoder) 
			aud = out->CreateTrack(&format, &mci);
		else
			aud = out->CreateTrack(&format);
			
		if (aud == NULL) {
			printf("Failed to create audio track\n");
			delete out;
			return;
		}
	}

	// Add the copyright and commit the header
	out->AddCopyright("Copyright 1999 Be Incorporated");
	out->CommitHeader();

	// Process the video track, if any
	if (vidtrack) {
		int is_key_frame = 0;
			
		if (found_video_encoder) {
			numFrames = vidtrack->CountFrames();
			for(j = 0; j < numFrames; j++) {
				int64 framecount = 1;
				printf("                                \r");
				printf("processing frame: %5Ld", j);
				fflush(stdout);
				err = vidtrack->ReadFrames(bitmap, &framecount, &mh);
				if (err) {
					printf("video: GetNextChunk error -- %s\n", strerror(err));
					break;
				}

				err = vid->WriteFrames(bitmap, 1, mh.u.encoded_video.field_flags);
				if (err) {
					printf("err %s (0x%x) writing video frame %Ld\n",
						   strerror(err), err, j);
					break;
				}
			}
		} else {
			numFrames = vidtrack->CountFrames();
			for(j = 0; j < numFrames; j++) {
				printf("                                \r");
				printf("processing frame: %5Ld", j);
				fflush(stdout);

				err = vidtrack->ReadChunk(&chunk, &sz, &mh);
				if (err) {
					printf("video: GetNextChunk error -- %s\n", strerror(err));
					break;
				}

				err = vid->WriteChunk(chunk, sz, mh.u.encoded_video.field_flags);
				if (err) {
					printf("err %s (0x%x) writing video frame %Ld\n",
						   strerror(err), err, j);
					break;
				}
			}
		}
		printf("\r                                     \r");
	}

	// Process the audio track, if any
	if (audtrack) {
		int64 		framecount = 0;

		if (found_audio_encoder) {
			// Decode and encode all the frames
			numFrames = audtrack->CountFrames();
			printf("Total frame count : %Ld\n", numFrames);
			for (j = 0; j < numFrames; j+=framecount) {
				err = audtrack->ReadFrames(sound_buffer, &framecount, &mh);
				if (err) {
					printf("video: GetNextChunk error -- %s\n", strerror(err));
					break;
				}
//				printf("processing frame: %lld-%lld\n", (int64)j, (int64)j-1+framecount);

				err = aud->WriteFrames(sound_buffer, framecount);
				if (err) {
					printf("err %s (0x%x) writing audio frame %Ld\n",
						   strerror(err), err, j);
					break;
				}
			}
		} else {
			// Just copy everything until we get an error. Could be improved.
			printf("processing chunks...\n");
			while (true) {
				err = audtrack->ReadFrames(sound_buffer, &framecount, &mh);
				if (err) {
					printf("audio: GetNextChunk error -- %s\n", strerror(err));
					break;
				}
	
				err = aud->WriteChunk(sound_buffer, framecount*framesize);
				if (err) {
					printf("err %s (0x%x) writing audio chunk %Ld\n",
						   strerror(err), err, j);
					break;
				}
			}
		}
		printf("\r                                     \r");
	}

	if (bitmap)
		free(bitmap);
	if (sound_buffer)
		free(sound_buffer);

	out->CloseFile();
	delete out;
	out = NULL;
}


void
dump_info(void)
{
	int32                  cookie = 0, cookie2;
	media_format 		   format, outfmt;
	media_file_format      mfi;
	media_codec_info     mci;


	while(get_next_file_format(&cookie, &mfi) == B_OK) {
		printf("%s (%s, id %d)\n", mfi.pretty_name,
			   mfi.short_name, mfi.id);

		cookie2 = 0;

		memset(&format, 0, sizeof(format));
		format.type = B_MEDIA_RAW_VIDEO;

		format.type = B_MEDIA_RAW_VIDEO;
		format.u.raw_video.last_active = (uint32)(320 - 1);
		format.u.raw_video.orientation = B_VIDEO_TOP_LEFT_RIGHT;
		format.u.raw_video.pixel_width_aspect = 1;
		format.u.raw_video.pixel_height_aspect = 3;
		format.u.raw_video.display.format = B_RGB32;
		format.u.raw_video.display.line_width = (int32)320;
		format.u.raw_video.display.line_count = (int32)240;
		format.u.raw_video.display.bytes_per_row = 4 * 320;

		printf("    Video Encoders:\n");
		while (get_next_encoder(&cookie2, &mfi, &format, &outfmt, &mci) == B_OK) {
			printf("        %s / %s (%d)\n", mci.pretty_name, mci.short_name, mci.id);
		}

		cookie2 = 0;
		format.type = B_MEDIA_RAW_AUDIO;
		format.u.raw_audio.format = media_raw_audio_format::B_AUDIO_UCHAR;
		format.u.raw_audio.channel_count = 1;
		printf("    Audio Encoders:\n");
		while (get_next_encoder(&cookie2, &mfi, &format, &outfmt, &mci) == B_OK) {
			printf("        %s / %s (%d)\n", mci.pretty_name, mci.short_name, mci.id);
		}
	}
}



int
main(int argc, char **argv)
{
	status_t		err;
	entry_ref       ref;
	media_format	format;
	BMediaFile		*mediaFile;
	BMediaTrack		*track = NULL, *vidtrack = NULL, *audtrack = NULL;
	int32			i, numTracks;
	char			*input = NULL, *output = NULL;
	char			*video_encoder_name = NULL, *audio_encoder_name = NULL;
	char            *family_name = NULL;

	if (argc < 2) {
		printf("usage: %s [-info][-avi|-qt][-wav][-aiff][-v <encoder_name>][-a <encoder_name>] <filename> [<output>]\n", argv[0]);
		return 1;
	}

	for (i=1; i < argc; i++) {
		if (strcmp(&argv[i][0], "-info") == 0) {
			dump_info();
			exit(0);
		} else if (strcmp(&argv[i][0], "-avi") == 0 ||
				   strcmp(&argv[i][0], "-wav") == 0 ||
				   strcmp(&argv[i][0], "-aiff") == 0 ||
				   strcmp(&argv[i][0], "-quicktime") == 0) {
			family_name = &argv[i][1];
		} else if (strcmp(&argv[i][0], "-qt") == 0) {
			family_name = "quicktime";
		} else if (strcmp(&argv[i][0], "-v") == 0 && argv[i+1]) {
			video_encoder_name = argv[i+1];
			i++;
		} else if (strcmp(&argv[i][0], "-a") == 0 && argv[i+1]) {
			audio_encoder_name = argv[i+1];
			i++;
		} else if (input == NULL) {
			input = &argv[i][0];
		} else if (output == NULL) {
			output = &argv[i][0];
		} else {
			printf("%s: extra argument %s\n", &argv[0][0], &argv[i][0]);
		}
	}
	
	if (output == NULL)
		output = "output";

	// get the ref from the path
	err = get_ref_for_path(input, &ref);
	if (err) {
		printf("problem with get_ref_for_path() -- %s\n", strerror(err));
		return 1;
	}

	// instanciate a BMediaFile object, and make sure there was no error.
	mediaFile = new BMediaFile(&ref);
	err = mediaFile->InitCheck();
	if (err) {
		printf("cannot contruct BMediaFile object -- %s\n", strerror(err));
		return 1;
	}

	numTracks = mediaFile->CountTracks();
	printf("%s has %d media tracks\n", input, numTracks);
	const char *copyright = mediaFile->Copyright();
	if (copyright)
		printf("#### copyright info: %s\n", copyright);
	
	for(i=0; i < numTracks; i++) {
		track = mediaFile->TrackAt(i);
		if (!track) {
			printf("cannot get track %d?!?\n", i);
			return 1;
		}

		// get the encoded format
		err = track->EncodedFormat(&format);
		if (err) {
			printf("BMediaTrack::EncodedFormat error -- %s\n", strerror(err));
			return 1;
		}

		if (format.type == B_MEDIA_RAW_VIDEO ||
			format.type == B_MEDIA_ENCODED_VIDEO) {
			
			vidtrack = track;
		} else if (format.type == B_MEDIA_RAW_AUDIO ||
				   format.type == B_MEDIA_ENCODED_AUDIO) {

			audtrack = track;
		} else {
			mediaFile->ReleaseTrack(track);
			track = NULL;
		}
	}

	if (vidtrack == NULL && audtrack == NULL) {
		printf("%s has no audio or video tracks?!?\n", input);
		return 1;
	}

	if (family_name == NULL && vidtrack == NULL)
		family_name = "wav";
	else if (family_name == NULL)
		family_name = "quicktime";

	transcode(vidtrack, audtrack, output, family_name,
				  video_encoder_name, audio_encoder_name);

	delete mediaFile;

	return 0;
}
