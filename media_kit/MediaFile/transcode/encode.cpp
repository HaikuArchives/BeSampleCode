
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#include <TranslationUtils.h>
#include <Bitmap.h>

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

static BBitmap *bm = NULL;

int getsize(const char *name, int *width, int *height)
{
	BBitmap *bm = BTranslationUtils::GetBitmapFile(name);
	if(bm){
		color_space cs = bm->ColorSpace();
		BRect rect = bm->Bounds();
		*width = rect.IntegerWidth() + 1;
		*height = rect.IntegerHeight() + 1;
		delete bm;
		if(cs == B_RGB32){
			return 0;
		} else {
			printf("file %s is not loadable as RGB32\n",name);
		}
	} else {
		return 1;
	}
}

char *readfile(char *name, int width, int height)
{
	static BBitmap *bm = NULL;
	
	if(bm) delete bm;
	width--;
	height--;
	
	bm = BTranslationUtils::GetBitmapFile(name);
	if(bm){
		BRect rect = bm->Bounds();
		if((width != rect.IntegerWidth()) ||
		(height != rect.IntegerHeight()) ||
		   (bm->ColorSpace() != B_RGB32)) {
			   printf("\nfile %s has different Bounds() or ColorSpace()\n",name);
			   return NULL;
		   }
		   
		return (char *) bm->Bits();
	} else {
		return NULL;
	}	
}

void
encode(char **files, char *output, media_format_family family, char *video_name)
{
	char					*chunk;
	char					*bitmap = NULL;
	bool            		found_video_encoder = false;
	bool					found_family;
	int32					i, sz, cookie;
	int32           		file_fmt_id = B_QUICKTIME_FORMAT_FAMILY;
	int64					numFrames, j;
	int64					framesize;
	status_t				err;
	entry_ref       		ref;
	BMediaFile      		*out;
	BMediaTrack    		 	*vid = NULL, *aud = NULL;
	media_format			format, outfmt;
	media_codec_info    	ei;
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
	while(get_next_file_format(&cookie, &mfi) == B_OK) {
		if (mfi.family == family) {
			goto writer_found;
		}
	}
	printf("failed to find a file format handler !\n");
	return;

	// Create and init the mediaFile
writer_found:
	out = new BMediaFile(&ref, &mfi);
	err = out->InitCheck();
	if (err != B_OK) {
		printf("failed to properly init the output file... (%s)\n", strerror(err));
		delete out;
		return;
	}

	int width, height;
	getsize(files[0],&width,&height);	
	BuildMediaFormat(width, height, B_RGB32, &format);
	
	if(video_name){
		cookie = 0;
		while (get_next_encoder(&cookie, &mfi, &format, &outfmt, &ei) == B_OK) {
			printf("found encoder %s (%d)\n", ei.pretty_name, ei.id);
			if (strcmp(video_name, ei.short_name) == 0) {
				found_video_encoder = true;
				break;
			}
		}
	}
	
	if(found_video_encoder){
		format.u.encoded_video.output.field_rate = 50000;
		vid = out->CreateTrack(&format, &ei);
	} else {
		format.u.raw_video.field_rate = 50000;
		vid = out->CreateTrack(&format);
	}
	
	if(!vid){
		printf("Failed to create video track\n");
		delete out;
		return;
	}

	// Add the copyright and commit the header
	//out->AddCopyright("Copyright 1999 Be Incorporated");
	out->CommitHeader();

	int is_key_frame = 0;
	
	for(j = 0; files[j]; j++) {
		printf("                                \r");
		printf("processing frame: %5Ld", j);
		fflush(stdout);
		
		if(!(bitmap = readfile(files[j], width, height))){
			printf("video: GetNextChunk error -- %d\n", j);
			break;
		}
					
/*		err = vidtrack->ReadFrames(bitmap, &framecount, &mh);
		if (err) {
			printf("video: GetNextChunk error -- %s\n", strerror(err));
			break;
		}*/
		
		err = vid->WriteFrames(bitmap, 1 /*, B_MEDIA_KEY_FRAME*/);
	
		if (err) {
			printf("err %s (0x%x) writing video frame %Ld\n",
			strerror(err), err, j);
			break;
		}
	}
	printf("\r                                     \r");
	
	out->CloseFile();
	delete out;
	out = NULL;
}

#define OUTPUT_AVI  1
#define OUTPUT_QT   2
#define OUTPUT_WAV	3
#define OUTPUT_AIFF	4

void
dump_info(void)
{
	int32                  cookie = 0, cookie2;
	media_format 		   format, outfmt;
	media_file_format      mfi;
	media_codec_info     ei;

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
		while (get_next_encoder(&cookie2, &mfi, &format, &outfmt, &ei) == B_OK) {
			printf("        %s / %s (%d)\n", ei.pretty_name, ei.short_name, ei.id);
		}
	}
}

int
main(int argc, char **argv)
{
	status_t		err;
	entry_ref       ref;
	media_format	format;
	char *output;
	int32			i, numTracks;
	char			**files = NULL;
	char			*video_encoder_name = NULL;
	int             which_mode = -1;
	BApplication    app("application/x-vnd.be-brians-encode");
	
	if (argc < 2) {
		printf("usage: %s [-info][-avi|-qt][-v <encoder_name>] [-o <output>] <file>...\n", argv[0]);
		return 1;
	}

	for (i=1; i < argc; i++) {
		if (strcmp(argv[i], "-info") == 0) {
			dump_info();
			exit(0);
		} else if (strcmp(argv[i], "-avi") == 0) {
			which_mode = OUTPUT_AVI;
		} else if (strcmp(argv[i], "-qt") == 0) {
			which_mode = OUTPUT_QT;
		} else if (strcmp(argv[i], "-v") == 0 && argv[i+1]) {
			video_encoder_name = argv[i+1];
			i++;
		} else if (strcmp(&argv[i][0], "-o") == 0 && argv[i+1]) {
			output = argv[i+1];
			i++;
		} else {
			int j = 0;
			files = (char **) malloc(sizeof(char *) * (argc - i + 1));
			while(i < argc) {
				files[j] = argv[i];
				j++;
				i++;
			}
			files[j] = NULL; 
		}
	}
	
	if(!files) {
		printf("no input files...\n");
		return 1;
	}
	
	if (output == NULL) output = "output";

	if(which_mode == -1) which_mode = OUTPUT_QT;

	if(which_mode == OUTPUT_QT) {
		encode(files, output, B_QUICKTIME_FORMAT_FAMILY, video_encoder_name);
	} else if (which_mode == OUTPUT_AVI) {
		encode(files, output, B_AVI_FORMAT_FAMILY, video_encoder_name);
	} else {
		printf("which_mode == %d (weird!)\n", which_mode);
	}

	return 0;
}
