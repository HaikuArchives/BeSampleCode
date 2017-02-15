/*
	Copyright 2000 Be Incorporated. All Rights Reserved.
	This file may be used under the terms of the Be Sample Code 
	License.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#include <Entry.h>
#include <GraphicsDefs.h>
#include <MediaDefs.h>
#include <MediaFormats.h>
#include <MediaFile.h>
#include <MediaTrack.h>

static inline void
print_format(const char *str, const media_format &fmt)
{
	static char __buf[128];
	printf("%s%s\n", str ? str : "",
		string_for_format((fmt), __buf, 128) ? __buf : "(failed)");
}

static uint32
bytes_per_row(color_space format, uint32 pixels)
{
	size_t pixel_chunk = 0;			// bytes per pixels_per_chunk pixels
	size_t row_alignment = 0;
	size_t pixels_per_chunk = 0;
	uint32 result = 0;
	status_t err;

	if(format == B_NO_COLOR_SPACE) {
		return 0;
	}

	err = get_pixel_size_for(
			format, &pixel_chunk, &row_alignment, &pixels_per_chunk);
	if(err != B_OK) {
		printf("bytes_per_row: get_pixel_size_for failed (%s)\n",
				strerror(err));
		return 0;	
	}

	// get num bytes
	result = ((pixels+pixels_per_chunk-1)/pixels_per_chunk) * pixel_chunk;
	// get num aligned bytes
	result = ((result+row_alignment-1) / row_alignment) * row_alignment;

	return result;
}


void
transcode(BMediaTrack *vidtrack, BMediaTrack *audtrack,
		  char *output, char *family_name, char *video_encoder_name,
		  char *audio_encoder_name, const char *copyright,
		  off_t start_frame, off_t end_frame)
{
	media_format			format, outfmt;
	media_codec_info    	mci;
	media_file_format		mfi;
	media_header 			mh;
	entry_ref       		ref;
	int64					clip_len, nframes, j = 0;
	bigtime_t				start_time = 0, end_time = 0;
	int64					framesize;
	int64					framecount;
	BMediaFile      		*out;
	BMediaTrack    		 	*vid = NULL, *aud = NULL;
	char					*chunk;
	char					*bitmap = NULL, *sound_buffer = NULL;
	float					vid_rate = 0.0;
	float					aud_rate = 0.0;
	int32					sz, cookie;
	status_t				err;
	bool            		found_video_encoder = false;
	bool					found_audio_encoder = false;

	// get the ref from the path
	err = get_ref_for_path(output, &ref);
	if (err) {
		printf("problem with get_ref_for_path() -- %s\n", strerror(err));
		return;
	}

	// find the right file-format handler
	cookie = 0;
	while((err = get_next_file_format(&cookie, &mfi)) == B_OK) {
		if (strcmp(mfi.short_name, family_name) == 0)
			break;
	}

	if (err != B_OK) {
		printf("failed to find a file format handler for '%s' (%s)\n",
				family_name, strerror(err));
		return;
	}

	// Create and init the mediaFile
	out = new BMediaFile(&ref, &mfi);
	err = out->InitCheck();
	if (err != B_OK) {
		printf("failed to properly init the output file... (%s)\n",
				strerror(err));
		delete out;
		return;
	}

	// Create the video track if any	
	if (vidtrack) {
		if (video_encoder_name) {
			uint32 bufsize;

			format.type = B_MEDIA_RAW_VIDEO;
			format.u.raw_video = media_raw_video_format::wildcard;
			vidtrack->DecodedFormat(&format);
			print_format("input video:  ", format);
			
			bufsize = format.Height()*format.u.raw_video.display.bytes_per_row;
			bitmap = (char *)malloc(bufsize);
			if(bitmap == NULL) {
				printf("can't allocate raw video buffer sized %ld\n", bufsize);
				delete out;
				return;
			}
	
			cookie = 0;
			while (get_next_encoder(&cookie, &mfi, &format, &outfmt, &mci)
					== B_OK)
			{
				if (strcmp(video_encoder_name, mci.short_name) == 0) {
					print_format("output video: ", outfmt);
					printf("found matching video encoder %s (%ld)\n",
							mci.pretty_name, mci.id);
					vid = out->CreateTrack(&format, &mci);
					found_video_encoder = true;
					break;
				}
			}
			if(!found_video_encoder) {
				printf("video encoder '%s' not found or incompatable "
						"with format\n", video_encoder_name);
				print_format("\t", format);
			}
		}
		else {
			vidtrack->EncodedFormat(&format);
			vid = out->CreateTrack(&format);
		}
			
		if (vid == NULL) {
			printf("Failed to create video track\n");
			delete out;
			return;
		}

		vid_rate = format.u.raw_video.field_rate;
	}

	// Create the audio track if any
	if (audtrack) {
		if (audio_encoder_name) {
			format.type = B_MEDIA_RAW_AUDIO;
			format.u.raw_audio = media_raw_audio_format::wildcard;
			audtrack->DecodedFormat(&format);

			sound_buffer = (char*)malloc(format.u.raw_audio.buffer_size);
			if(sound_buffer == NULL) {
				printf("can't allocate audio buffer sized %ld\n",
						format.u.raw_audio.buffer_size);
				goto cleanup;
			}

			// as we all know, the low 4 bits of format
			// is guaranteed to be the size of a sample
			framesize = (format.AudioFormat() & 0xf) *
					format.u.raw_audio.channel_count;
	
			cookie = 0;
			while (get_next_encoder(&cookie, &mfi, &format, &outfmt, &mci)
					== B_OK)
			{
				if (strcmp(audio_encoder_name, mci.short_name) == 0) {
					printf("found matching audio encoder %s (%ld)\n",
							mci.pretty_name, mci.id);
					aud = out->CreateTrack(&format, &mci);
					found_audio_encoder = true;
					break;
				}
			}
			if(!found_audio_encoder) {
				printf("audio encoder '%s' not found or incompatable "
						"with format\n", audio_encoder_name);
				print_format("\t", format);
			}
		}
		else {
			audtrack->EncodedFormat(&format);
			aud = out->CreateTrack(&format);
		}
			
		if (aud == NULL) {
			printf("Failed to create audio track\n");
			goto cleanup;
		}

		aud_rate = format.u.raw_audio.frame_rate;
	}

	// Add the copyright and commit the header
	if(copyright == NULL)
		copyright = "Copyright 2000 Be Incorporated";
	out->AddCopyright(copyright);
	out->CommitHeader();

	// Process the video track, if any
	if (vidtrack) {
		if (found_video_encoder) {
//! the audio track should be clipped to the same length as the video track
//  save the first/last times so we can seek to them in the audio track
			if (start_frame > 0) {
				status_t res;
				off_t frame = start_frame;
			
				res = vidtrack->SeekToFrame(&frame);
				if (frame != start_frame) {
					printf("Start frame adjusted from %Ld to %Ld\n",
							start_frame, frame);
					if (frame < 0) {
						printf("DANGER! BAD MEDIA EXTRACTOR RETURNED A "
							   "NEGATIVE FRAME (%Ld)!!!!\n", frame);
						frame = 0;
					}
					start_frame = frame;
				}
			} else {
				start_frame = 0;
			}

			start_time = (bigtime_t)(start_frame * 1e6 / vid_rate);

			if (end_frame > 0) {
				clip_len = end_frame - start_frame;
				if (clip_len <= 0)
					clip_len = 1;
				nframes = clip_len;
			}
			else {
				clip_len = vidtrack->CountFrames();
				nframes = B_INFINITE_TIMEOUT;
			}
			
			for(j = 0; j < nframes; j+=framecount) {
				printf("                                \r");
				printf("processing frame: %5Ld / %5Ld", j, clip_len);
				fflush(stdout);

				err = vidtrack->ReadFrames(bitmap, &framecount, &mh);
				if (err) {
					if(err != B_LAST_BUFFER_ERROR) {
						printf("video: GetNextChunk error -- %s\n",
								strerror(err));
					}
					break;
				}

				err = vid->WriteFrames(bitmap, framecount,
						mh.u.encoded_video.field_flags);
				if (err) {
					printf("error writing video frame %Ld -- %s\n",
						   j, strerror(err));
					break;
				}
			}

			end_time = (bigtime_t)((start_frame + j) * 1e6 / vid_rate);
printf("\ntranscoded video clip %Ld..%Ld (%Ldus)\n", start_time, end_time, end_time-start_time);
		} else {
			clip_len = vidtrack->CountFrames();
			for(j = 0; ; j++) {
				printf("                                \r");
				printf("processing chunk: %5Ld / %5Ld", j, clip_len);
				fflush(stdout);

				err = vidtrack->ReadChunk(&chunk, &sz, &mh);
				if (err) {
					if(err != B_LAST_BUFFER_ERROR) {
						printf("video: GetNextChunk error -- %s\n",
								strerror(err));
					}
					break;
				}

				err = vid->WriteChunk(chunk, sz,
						mh.u.encoded_video.field_flags);
				if (err) {
					printf("error writing video chunk %Ld -- %s\n",
						   j, strerror(err));
					break;
				}
			}
		}
		printf("\r                                     \r");
	}

	// Process the audio track, if any
	if (audtrack) {
		int64 nframes;

		if (found_audio_encoder) {

			clip_len = audtrack->CountFrames();
			nframes = B_INFINITE_TIMEOUT;

			// don't clip the track unless told to do so
			if(start_frame != 0 || end_frame != 0) {
				bigtime_t duration;

				if(start_time != 0) {
					bigtime_t time = start_time;
				
					audtrack->SeekToTime(&time);
					if (time != start_time) {
						printf("Start time adjusted from %Ld to %Ld\n",
								start_time, time);
						if (time < 0) {
							printf("DANGER! BAD MEDIA EXTRACTOR RETURNED A "
								   "NEGATIVE TIME (%Ld)!!!!\n", time);
							time = 0;
						}
						start_time = time;
					}
				} else {
					start_time = 0;
				}

				duration = end_time - start_time;
				if(duration > 0) {
					clip_len = (int64)(aud_rate * duration / 1e6);
					nframes = clip_len;
				}
			}

			printf("Total audio frame count : %Ld\n", clip_len);
			for (j = 0; j < nframes; j+=framecount) {
				printf("                                \r");
				printf("processing frame: %5Ld / %5Ld", j, clip_len);
				fflush(stdout);

				err = audtrack->ReadFrames(sound_buffer, &framecount, &mh);
				if (err) {
					if(err != B_LAST_BUFFER_ERROR) {
						printf("video: GetNextChunk error -- %s\n",
								strerror(err));
					}
					break;
				}

				err = aud->WriteFrames(sound_buffer, framecount);
				if (err) {
					printf("err %s (0x%lx) writing audio frame %Ld\n",
						   strerror(err), err, j);
					break;
				}
			}
printf("\ntranscoded audio clip %Ld..%Ld (%Ldus)\n", start_time, end_time, end_time-start_time);
		} else {
			// Just copy the audio data as-is
			printf("processing chunks...\n");
			while (true) {
				err = audtrack->ReadChunk(&chunk, &sz, &mh);
				if (err) {
					if(err != B_LAST_BUFFER_ERROR) {
						printf("audio: GetNextChunk error -- %s\n",
								strerror(err));
					}
					break;
				}
	
				err = aud->WriteChunk(chunk, sz);
				if (err) {
					printf("err %s (0x%lx) writing audio chunk %Ld\n",
						   strerror(err), err, j);
					break;
				}
			}
		}
		printf("\r                                     \r");
	}

cleanup:
	if (bitmap)
		free(bitmap);
	if (sound_buffer)
		free(sound_buffer);

	if(out) {
		out->CloseFile();
		delete out;
	}
}


void
dump_info(void)
{
	int32                  cookie = 0, cookie2;
	media_format 		   format, outfmt;
	media_file_format      mfi;
	media_codec_info     mci;


	while(get_next_file_format(&cookie, &mfi) == B_OK) {
		printf("%s (%s, id %lu)\n",
				mfi.pretty_name, mfi.short_name, mfi.id.internal_id);

		format.type = B_MEDIA_RAW_VIDEO;
		format.u.raw_video = media_raw_video_format::wildcard;

		printf("    Video Encoders:\n");
		cookie2 = 0;
		while (get_next_encoder(&cookie2, &mfi, &format, &outfmt, &mci) == B_OK)
		{
			printf("        %s / %s (%ld)\n",
					mci.pretty_name, mci.short_name, mci.id);
		}

		format.type = B_MEDIA_RAW_AUDIO;
		format.u.raw_audio = media_raw_audio_format::wildcard;

		printf("    Audio Encoders:\n");
		cookie2 = 0;
		while (get_next_encoder(&cookie2, &mfi, &format, &outfmt, &mci) == B_OK)
		{
			printf("        %s / %s (%ld)\n",
					mci.pretty_name, mci.short_name, mci.id);
		}
	}
}


void
usage(const char *argv0)
{
	printf("usage: %s [-start frame-num][-end frame-num][-info]"
			"[-avi|-qt|-quicktime|-wav|-aiff|-mp3]"
			"[-v <encoder_name>][-a <encoder_name>] "
			"<filename> [<output>]\n", argv0);
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
	off_t           start_frame = -1, end_frame = -1;
	const char		*copyright;

	if (argc < 2) {
		usage(argv[0]);
		return 1;
	}

	for (i=1; i < argc; i++) {
		if (strcmp(&argv[i][0], "-info") == 0) {
			dump_info();
			exit(0);
		} else if (strcmp(argv[i], "-avi") == 0 ||
				   strcmp(argv[i], "-wav") == 0 ||
				   strcmp(argv[i], "-aiff") == 0 ||
				   strcmp(argv[i], "-mp3") == 0 ||
				   strcmp(argv[i], "-quicktime") == 0) {
			family_name = &argv[i][1];
		} else if (strcmp(argv[i], "-qt") == 0) {
			family_name = "quicktime";
		} else if (strcmp(argv[i], "-v") == 0 && argv[i+1]) {
			video_encoder_name = argv[i+1];
			i++;
		} else if (strcmp(argv[i], "-a") == 0 && argv[i+1]) {
			audio_encoder_name = argv[i+1];
			i++;
		} else if (strcmp(argv[i], "-start") == 0 && argv[i+1]) {
			start_frame = strtoll(argv[i+1], NULL, 0);
			i++;
		} else if (strcmp(argv[i], "-end") == 0 && argv[i+1]) {
			end_frame = strtoll(argv[i+1], NULL, 0);
			i++;
		} else if (input == NULL) {
			input = argv[i];
		} else if (output == NULL) {
			output = argv[i];
		} else {
			printf("%s: extra argument %s\n", argv[0], argv[i]);
		}
	}
	
	if (input == NULL) {
		printf("%s: input filename required\n", argv[0]);
		usage(argv[0]);
		return 1;
	}

	if (output == NULL)
		output = "output";

	// get the ref from the path
	err = get_ref_for_path(input, &ref);
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

	numTracks = mediaFile->CountTracks();
	printf("%s has %ld media tracks\n", input, numTracks);
	copyright = mediaFile->Copyright();
	if (copyright)
		printf("#### copyright info: %s\n", copyright);
	
	for(i=0; i < numTracks; i++) {
		track = mediaFile->TrackAt(i);
		if (!track) {
			printf("cannot get track %ld?!?\n", i);
			return 1;
		}

		err = track->EncodedFormat(&format);
		if (err) {
			printf("BMediaTrack::EncodedFormat error -- %s\n", strerror(err));
			return 1;
		}

		if (format.IsVideo()) {
			vidtrack = track;
		} else if (format.IsAudio()) {
			audtrack = track;
		} else {
			printf("unknown type %d at track %ld\n", format.type, i);
			mediaFile->ReleaseTrack(track);
			track = NULL;
		}
	}

	if (vidtrack == NULL && audtrack == NULL) {
		printf("%s has no audio or video tracks\n", input);
		return 1;
	}

	if (family_name == NULL && vidtrack == NULL)
		family_name = "wav";
	else if (family_name == NULL)
		family_name = "quicktime";

	transcode(vidtrack, audtrack, output, family_name,
			  video_encoder_name, audio_encoder_name,
			  copyright, start_frame, end_frame);

	delete mediaFile;

	return 0;
}
