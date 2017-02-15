/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

/*	Parse the ASCII and raw versions of PPM.	*/
/*	Note that the parsing of ASCII is very inefficient, because BFile	*/
/*	does not buffer data. We should wrap a buffering thing around	*/
/*	the input or output when they are in ASCII mode.				*/

#include <TranslatorAddOn.h>
#include <TranslationKit.h>
#include <ByteOrder.h>
#include <Message.h>
#include <Screen.h>
#include <Locker.h>
#include <FindDirectory.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <CheckBox.h>
#include <Bitmap.h>

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "colorspace.h"


#if DEBUG
 #define dprintf(x) printf x
#else
 #define dprintf(x)
#endif


#if !defined(_PR3_COMPATIBLE_)	/* R4 headers? Else we need to define these constants. */
 #define B_CMY24 ((color_space)0xC001) /* C[7:0]  M[7:0]  Y[7:0]                       No gray removal done            */
 #define B_CMY32 ((color_space)0xC002) /* C[7:0]  M[7:0]  Y[7:0]  X[7:0]       No gray removal done            */
 #define B_CMYA32 ((color_space)0xE002) /* C[7:0]  M[7:0]  Y[7:0]  A[7:0]       No gray removal done            */
 #define B_CMYK32 ((color_space)0xC003) /* C[7:0]  M[7:0]  Y[7:0]  K[7:0]                                       */
#endif


/* These three data items are exported by every translator. */
char translatorName[] = "PPMTranslator";
char translatorInfo[] = "Reads and writes images in the PPM file format. http://www.be.com/";
int32 translatorVersion = 100; /* format is revision+minor*10+major*100 */


/*	Be reserves all codes with non-lowecase letters in them.	*/
/*	Luckily, there is already a reserved code for PPM. If you	*/
/*	make up your own for a new type, use lower-case letters.	*/
#define PPM_TYPE 'PPM '


/* These two data arrays are a really good idea to export from Translators, but not required. */
translation_format inputFormats[] = {
	{	B_TRANSLATOR_BITMAP, B_TRANSLATOR_BITMAP, 0.4, 0.8, "image/x-be-bitmap", "Be Bitmap Format (PPMHandler)" },
	{	PPM_TYPE, B_TRANSLATOR_BITMAP, 0.3, 0.8, "image/x-ppm", "PPM portable pixmap format" },
	{	0, 0, 0, 0, "\0", "\0" }
};		/*	optional (else Identify is always called)	*/

translation_format outputFormats[] = {
	{	B_TRANSLATOR_BITMAP, B_TRANSLATOR_BITMAP, 0.4, 0.8, "image/x-be-bitmap", "Be Bitmap Format (PPMHandler)" },
	{	PPM_TYPE, B_TRANSLATOR_BITMAP, 0.3, 0.8, "image/x-ppm", "PPM portable pixmap format" },
	{	0, 0, 0, 0, "\0", "\0" }
};	/*	optional (else Translate is called anyway)	*/

/*	Translators that don't export outputFormats 	*/
/*	will not be considered by files looking for 	*/
/*	specific output formats.	*/


/*	We keep our settings in a global struct, and wrap a lock around them.	*/
struct ppm_settings {
	color_space		out_space;
	BPoint				window_pos;
	bool				write_ascii;
	bool				settings_touched;
};
BLocker g_settings_lock("PPM settings lock");
ppm_settings g_settings;

BPoint get_window_origin();
void set_window_origin(BPoint pos);
BPoint get_window_origin()
{
	BPoint ret;
	g_settings_lock.Lock();
	ret = g_settings.window_pos;
	g_settings_lock.Unlock();
	return ret;
}

void set_window_origin(BPoint pos)
{
	g_settings_lock.Lock();
	g_settings.window_pos = pos;
	g_settings.settings_touched = true;
	g_settings_lock.Unlock();
}


class PrefsLoader {
public:
		PrefsLoader(
				const char * str)
			{
				dprintf(("PPMTranslator: PrefsLoader()\n"));
				/* defaults */
				g_settings.out_space = B_NO_COLOR_SPACE;
				g_settings.window_pos = B_ORIGIN;
				g_settings.write_ascii = false;
				g_settings.settings_touched = false;
				BPath path;
				if (find_directory(B_USER_SETTINGS_DIRECTORY, &path)) {
					path.SetTo("/tmp");
				}
				path.Append(str);
				FILE * f = fopen(path.Path(), "r");
				/*	parse text settings file -- this should be a library...	*/
				if (f) {
					char line[1024];
					char name[32];
					char * ptr;
					while (true) {
						line[0] = 0;
						fgets(line, 1024, f);
						if (!line[0]) {
							break;
						}
						/* remember: line ends with \n, so printf()s don't have to */
						ptr = line;
						while (isspace(*ptr)) {
							ptr++;
						}
						if (*ptr == '#' || !*ptr) {	/* comment or blank */
							continue;
						}
						if (sscanf(ptr, "%31[a-zA-Z_0-9] =", name) != 1) {
							fprintf(stderr, "unknown PPMTranslator settings line: %s", line);
						}
						else {
							if (!strcmp(name, "color_space")) {
								while (*ptr != '=') {
									ptr++;
								}
								ptr++;
								if (sscanf(ptr, "%d", (int*)&g_settings.out_space) != 1) {
									fprintf(stderr, "illegal color space in PPMTranslator settings: %s", ptr);
								}
							}
							else if (!strcmp(name, "window_pos")) {
								while (*ptr != '=') {
									ptr++;
								}
								ptr++;
								if (sscanf(ptr, "%f,%f", &g_settings.window_pos.x, &g_settings.window_pos.y) != 2) {
									fprintf(stderr, "illegal window position in PPMTranslator settings: %s", ptr);
								}
							}
							else if (!strcmp(name, "write_ascii")) {
								while (*ptr != '=') {
									ptr++;
								}
								ptr++;
								int ascii = g_settings.write_ascii;
								if (sscanf(ptr, "%d", &ascii) != 1) {
									fprintf(stderr, "illegal write_ascii value in PPMTranslator settings: %s", ptr);
								}
								else {
									g_settings.write_ascii = ascii;
								}
							}
							else {
								fprintf(stderr, "unknown PPMTranslator setting: %s", line);
							}
						}
					}
					fclose(f);
				}
			}
		~PrefsLoader()
			{
				/*	No need writing settings if there aren't any	*/
				if (g_settings.settings_touched) {
					BPath path;
					if (find_directory(B_USER_SETTINGS_DIRECTORY, &path)) {
						path.SetTo("/tmp");
					}
					path.Append("PPMTranslator_Settings");
					FILE * f = fopen(path.Path(), "w");
					if (f) {
						fprintf(f, "# PPMTranslator settings version %.2f\n", (float)translatorVersion/100.0);
						fprintf(f, "color_space = %d\n", g_settings.out_space);
						fprintf(f, "window_pos = %g,%g\n", g_settings.window_pos.x, g_settings.window_pos.y);
						fprintf(f, "write_ascii = %d\n", g_settings.write_ascii ? 1 : 0);
						fclose(f);
					}
				}
			}
};

PrefsLoader g_prefs_loader("PPMTranslator_Settings");

/*	Some prototypes for functions we use.	*/
status_t read_ppm_header(BDataIO * io, int * width, int * rowbytes, int * height, 
	int * max, bool * ascii, color_space * space, bool * is_ppm, char ** comment);
status_t read_bits_header(BDataIO * io, int skipped, int * width, int * rowbytes, 
	int * height, int * max, bool * ascii, color_space * space);
status_t write_comment(const char * str, BDataIO * io);
status_t copy_data(BDataIO * in, BDataIO * out, int rowbytes, int out_rowbytes, 
	int height, int max, bool in_ascii, bool out_ascii, color_space in_space, 
	color_space out_space);

	/*	Return B_NO_TRANSLATOR if not handling this data.	*/
	/*	Even if inputFormats exists, may be called for data without hints	*/
	/*	If outType is not 0, must be able to export in wanted format	*/

status_t
Identify(	/*	required	*/
	BPositionIO * inSource,
	const translation_format * inFormat,	/*	can beNULL	*/
	BMessage * ioExtension,	/*	can be NULL	*/
	translator_info * outInfo,
	uint32 outType)
{
	dprintf(("PPMTranslator: Identify()\n"));
	/* Silence compiler warnings. */
	inFormat = inFormat;
	ioExtension = ioExtension;

	/* Check that requested format is something we can deal with. */
	if (outType == 0) {
		outType = B_TRANSLATOR_BITMAP;
	}
	if (outType != B_TRANSLATOR_BITMAP && outType != PPM_TYPE) {
		return B_NO_TRANSLATOR;
	}

	/* Check header. */
	int width, rowbytes, height, max;
	bool ascii, is_ppm;
	color_space space;
	status_t err = read_ppm_header(inSource, &width, &rowbytes, &height, &max, &ascii, &space, &is_ppm, NULL);
	if (err != B_OK) {
		return err;
	}
	/* Stuff info into info struct -- Translation Kit will do "translator" for us. */
	outInfo->group = B_TRANSLATOR_BITMAP;
	if (is_ppm) {
		outInfo->type = PPM_TYPE;
		outInfo->quality = 0.3;		/* no alpha, etc */
		outInfo->capability = 0.8;	/* we're pretty good at PPM reading, though */
		strcpy(outInfo->name, "PPM portable pixmap format");
		strcpy(outInfo->MIME, "image/x-ppm");
	}
	else {
		outInfo->type = B_TRANSLATOR_BITMAP;
		outInfo->quality = 0.4;		/* B_TRANSLATOR_BITMAP can do alpha, at least */
		outInfo->capability = 0.8;	/* and we might not know many variations thereof */
		strcpy(outInfo->name, "Be Bitmap Format (PPMHandler)");
		strcpy(outInfo->MIME, "image/x-be-bitmap");	/* this is the MIME type of B_TRANSLATOR_BITMAP */
	}
	return B_OK;
}


	/*	Return B_NO_TRANSLATOR if not handling the output format	*/
	/*	If outputFormats exists, will only be called for those formats	*/

status_t
Translate(	/*	required	*/
	BPositionIO * inSource,
	const translator_info * /* inInfo*/ ,	/* silence compiler warning */
	BMessage * ioExtension,	/*	can be NULL	*/
	uint32 outType,
	BPositionIO * outDestination)
{
	dprintf(("PPMTranslator: Translate()\n"));
	inSource->Seek(0, SEEK_SET);	/* paranoia */
//	inInfo = inInfo;	/* silence compiler warning */
	/* Check what we're being asked to produce. */
	if (!outType) {
		outType = B_TRANSLATOR_BITMAP;
	}
	dprintf(("PPMTranslator: outType is '%c%c%c%c'\n", char(outType>>24), char(outType>>16), char(outType>>8), char(outType)));
	if (outType != B_TRANSLATOR_BITMAP && outType != PPM_TYPE) {
		return B_NO_TRANSLATOR;
	}

	/* Figure out what we've been given (again). */
	int width, rowbytes, height, max;
	bool ascii, is_ppm;
	color_space space;
	/* Read_ppm_header() will always return with stream at beginning of data */
	/* for both B_TRANSLATOR_BITMAP and PPM_TYPE, and indicate what it is. */
	char * comment = NULL;
	status_t err = read_ppm_header(inSource, &width, &rowbytes, &height, &max, &ascii, &space, &is_ppm, &comment);
	if (comment != NULL) {
		if (ioExtension != NULL) {
#if defined(_PR3_COMPATIBLE_)	/* R4 headers? */
			ioExtension->AddString(B_TRANSLATOR_EXT_COMMENT, comment);
#else
			ioExtension->AddString("/comment", comment);
#endif
		}
		free(comment);
	}
	if (err < B_OK) {
		dprintf(("read_ppm_header() error %s [%lx]\n", strerror(err), err));
		return err;
	}
	/* Check if we're configured to write ASCII format file. */
	bool out_ascii = false;
	if (outType == PPM_TYPE) {
		out_ascii = g_settings.write_ascii;
		if (ioExtension != NULL) {
			ioExtension->FindBool("ppm /ascii", &out_ascii);
		}
	}
	err = B_OK;
	/* Figure out which color space to convert to */
	color_space out_space;
	int out_rowbytes;
	g_settings_lock.Lock();
	if (outType == PPM_TYPE) {
		out_space = B_RGB24_BIG;
		out_rowbytes = 3*width;
	}
	else {	/*	When outputting to B_TRANSLATOR_BITMAP, follow user's wishes.	*/
#if defined(_PR3_COMPATIBLE_)	/* R4 headers? */
		if (!ioExtension || ioExtension->FindInt32(B_TRANSLATOR_EXT_BITMAP_COLOR_SPACE, (int32*)&out_space) || 
#else
		if (!ioExtension || ioExtension->FindInt32("bits/space", (int32*)&out_space) || 
#endif
			(out_space == B_NO_COLOR_SPACE)) {
			if (g_settings.out_space == B_NO_COLOR_SPACE) {
				switch (space) {	/*	The 24-bit versions are pretty silly, use 32 instead.	*/
				case B_RGB24:
				case B_RGB24_BIG:
					out_space = B_RGB32;
					break;
				default:
					/* use whatever is there */
					out_space = space;
					break;
				}
			}
			else {
				out_space = g_settings.out_space;
			}
		}
		out_rowbytes = calc_rowbytes(out_space, width);
	}
	g_settings_lock.Unlock();
	/* Write file header */
	if (outType == PPM_TYPE) {
		dprintf(("PPMTranslator: write PPM\n"));
		/* begin PPM header */
		const char * magic;
		if (out_ascii)
			magic = "P3\n";
		else
			magic = "P6\n";
		err = outDestination->Write(magic, strlen(magic));
		if (err == (long)strlen(magic)) err = 0;
		//comment = NULL;
		const char* fsComment;
#if defined(_PR3_COMPATIBLE_)	/* R4 headers? */
		if ((ioExtension != NULL) && !ioExtension->FindString(B_TRANSLATOR_EXT_COMMENT, &fsComment)) {
#else
		if ((ioExtension != NULL) && !ioExtension->FindString("/comment", &fsComment)) {
#endif
			err = write_comment(fsComment, outDestination);
		}
		if (err == B_OK) {
			char data[40];
			sprintf(data, "%d %d %d\n", width, height, max);
			err = outDestination->Write(data, strlen(data));
			if (err == (long)strlen(data)) err = 0;
		}
		/* header done */
	}
	else {
		dprintf(("PPMTranslator: write B_TRANSLATOR_BITMAP\n"));
		/* B_TRANSLATOR_BITMAP header */
		TranslatorBitmap hdr;
		hdr.magic = B_HOST_TO_BENDIAN_INT32(B_TRANSLATOR_BITMAP);
		hdr.bounds.left = B_HOST_TO_BENDIAN_FLOAT(0);
		hdr.bounds.top = B_HOST_TO_BENDIAN_FLOAT(0);
		hdr.bounds.right = B_HOST_TO_BENDIAN_FLOAT(width-1);
		hdr.bounds.bottom = B_HOST_TO_BENDIAN_FLOAT(height-1);
		hdr.rowBytes = B_HOST_TO_BENDIAN_INT32(out_rowbytes);
		hdr.colors = (color_space)B_HOST_TO_BENDIAN_INT32(out_space);
		hdr.dataSize = B_HOST_TO_BENDIAN_INT32(out_rowbytes*height);
		dprintf(("rowBytes is %d, width %d, out_space %x, space %x\n", out_rowbytes, width, out_space, space));
		err = outDestination->Write(&hdr, sizeof(hdr));
		dprintf(("PPMTranslator: Write() returns %lx\n", err));
#if DEBUG
		{
			BBitmap * map = new BBitmap(BRect(0,0,width-1,height-1), out_space);
			printf("map rb = %ld\n", map->BytesPerRow());
			delete map;
		}
#endif
		if (err == sizeof(hdr)) err = 0;
		/* header done */
	}
	if (err != B_OK) {
		return err > 0 ? B_IO_ERROR : err;
	}
	/* Write data. Luckily, PPM and B_TRANSLATOR_BITMAP both scan from left to right, top to bottom. */
	return copy_data(inSource, outDestination, rowbytes, out_rowbytes, height, max, ascii, out_ascii, space, out_space);
}


class PPMView :
	public BView
{
public:
		PPMView(
				const BRect & frame,
				const char * name,
				uint32 resize,
				uint32 flags) :
			BView(frame, name, resize, flags)
			{
				SetViewColor(220,220,220,0);
				mMenu = new BPopUpMenu("Color Space");
				mMenu->AddItem(new BMenuItem("None", CSMessage(B_NO_COLOR_SPACE)));
				mMenu->AddItem(new BMenuItem("RGB 8:8:8 32 bits", CSMessage(B_RGB32)));
				mMenu->AddItem(new BMenuItem("RGBA 8:8:8:8 32 bits", CSMessage(B_RGBA32)));
				mMenu->AddItem(new BMenuItem("RGB 5:5:5 16 bits", CSMessage(B_RGB15)));
				mMenu->AddItem(new BMenuItem("RGBA 5:5:5:1 16 bits", CSMessage(B_RGBA15)));
				mMenu->AddItem(new BMenuItem("RGB 5:6:5 16 bits", CSMessage(B_RGB16)));
				mMenu->AddItem(new BMenuItem("System Palette 8 bits", CSMessage(B_CMAP8)));
				mMenu->AddSeparatorItem();
				mMenu->AddItem(new BMenuItem("Grayscale 8 bits", CSMessage(B_GRAY8)));
				mMenu->AddItem(new BMenuItem("Bitmap 1 bit", CSMessage(B_GRAY1)));
				mMenu->AddItem(new BMenuItem("CMY 8:8:8 32 bits", CSMessage(B_CMY32)));
				mMenu->AddItem(new BMenuItem("CMYA 8:8:8:8 32 bits", CSMessage(B_CMYA32)));
				mMenu->AddItem(new BMenuItem("CMYK 8:8:8:8 32 bits", CSMessage(B_CMYK32)));
				mMenu->AddSeparatorItem();
				mMenu->AddItem(new BMenuItem("RGB 8:8:8 32 bits big-endian", CSMessage(B_RGB32_BIG)));
				mMenu->AddItem(new BMenuItem("RGBA 8:8:8:8 32 bits big-endian", CSMessage(B_RGBA32_BIG)));
				mMenu->AddItem(new BMenuItem("RGB 5:5:5 16 bits big-endian", CSMessage(B_RGB15_BIG)));
				mMenu->AddItem(new BMenuItem("RGBA 5:5:5:1 16 bits big-endian", CSMessage(B_RGBA15_BIG)));
				mMenu->AddItem(new BMenuItem("RGB 5:6:5 16 bits big-endian", CSMessage(B_RGB16)));
				mField = new BMenuField(BRect(20,70,180,90), "Color Space Field", "Color Space", mMenu);
				mField->SetViewColor(ViewColor());
				AddChild(mField);
				SelectColorSpace(g_settings.out_space);
				BMessage * msg = new BMessage(CHANGE_ASCII);
				mAscii = new BCheckBox(BRect(20,45,180,62), "Write ASCII", "Write ASCII", msg);
				if (g_settings.write_ascii) {
					mAscii->SetValue(1);
				}
				mAscii->SetViewColor(ViewColor());
				AddChild(mAscii);
			}
		~PPMView()
			{
				/* nothing here */
			}

		enum {
			SET_COLOR_SPACE = 'ppm=',
			CHANGE_ASCII
		};

virtual	void Draw(
				BRect area)
			{
				area = area; /* silence compiler */
				SetFont(be_bold_font);
				font_height fh;
				GetFontHeight(&fh);
				char str[100];
				sprintf(str, "PPMTranslator %.2f", (float)translatorVersion/100.0);
				DrawString(str, BPoint(fh.descent+1, fh.ascent+fh.descent*2+fh.leading));
			}
virtual	void MessageReceived(
				BMessage * message)
			{
				if (message->what == SET_COLOR_SPACE) {
					SetSettings(message);
				}
				else if (message->what == CHANGE_ASCII) {
					BMessage msg;
					msg.AddBool("ppm /ascii", mAscii->Value());
					SetSettings(&msg);
				}
				else {
					BView::MessageReceived(message);
				}
			}
virtual	void AllAttached()
			{
				BView::AllAttached();
				BMessenger msgr(this);
				/*	Tell all menu items we're the man.	*/
				for (int ix=0; ix<mMenu->CountItems(); ix++) {
					BMenuItem * i = mMenu->ItemAt(ix);
					if (i) {
						i->SetTarget(msgr);
					}
				}
				mAscii->SetTarget(msgr);
			}

		void SetSettings(
				BMessage * message)
			{
				g_settings_lock.Lock();
				color_space space;
				if (!message->FindInt32("space", (int32*)&space)) {
					g_settings.out_space = space;
					SelectColorSpace(space);
					g_settings.settings_touched = true;
				}
				bool ascii;
				if (!message->FindBool("ppm /ascii", &ascii)) {
					g_settings.write_ascii = ascii;
					g_settings.settings_touched = true;
				}
				g_settings_lock.Unlock();
			}

private:
		BPopUpMenu * mMenu;
		BMenuField * mField;
		BCheckBox * mAscii;

		BMessage * CSMessage(
				color_space space)
			{
				BMessage * ret = new BMessage(SET_COLOR_SPACE);
				ret->AddInt32("space", space);
				return ret;
			}

		void SelectColorSpace(
				color_space space)
			{
				for (int ix=0; ix<mMenu->CountItems(); ix++) {
					int32 s;
					BMenuItem * i = mMenu->ItemAt(ix);
					if (i) {
						BMessage * m = i->Message();
						if (m && !m->FindInt32("space", &s) && (s == space)) {
							mMenu->Superitem()->SetLabel(i->Label());
							break;
						}
					}
				}
			}
};


	/*	The view will get resized to what the parent thinks is 	*/
	/*	reasonable. However, it will still receive MouseDowns etc.	*/
	/*	Your view should change settings in the translator immediately, 	*/
	/*	taking care not to change parameters for a translation that is 	*/
	/*	currently running. Typically, you'll have a global struct for 	*/
	/*	settings that is atomically copied into the translator function 	*/
	/*	as a local when translation starts.	*/
	/*	Store your settings wherever you feel like it.	*/

status_t 
MakeConfig(	/*	optional	*/
	BMessage * ioExtension,	/*	can be NULL	*/
	BView * * outView,
	BRect * outExtent)
{
	PPMView * v = new PPMView(BRect(0,0,200,100), "PPMTranslator Settings", B_FOLLOW_ALL, B_WILL_DRAW);
	*outView = v;
	*outExtent = v->Bounds();
	if (ioExtension) {
		v->SetSettings(ioExtension);
	}
	return B_OK;
}


	/*	Copy your current settings to a BMessage that may be passed 	*/
	/*	to BTranslators::Translate at some later time when the user wants to 	*/
	/*	use whatever settings you're using right now.	*/

status_t
GetConfigMessage(	/*	optional	*/
	BMessage * ioExtension)
{
	status_t err = B_OK;
#if defined(_PR3_COMPATIBLE_)
	const char * name = B_TRANSLATOR_EXT_BITMAP_COLOR_SPACE;
#else
	const char * name = "bits/space";
#endif
	g_settings_lock.Lock();
	(void)ioExtension->RemoveName(name);
	err = ioExtension->AddInt32(name, g_settings.out_space);
	g_settings_lock.Unlock();
	return err;
}





status_t
read_ppm_header(
	BDataIO * inSource,
	int * width,
	int * rowbytes,
	int * height, 
	int * max,
	bool * ascii,
	color_space * space,
	bool * is_ppm,
	char ** comment)
{
	/* check for PPM magic number */
	char ch[2];
	if (inSource->Read(ch, 2) != 2) {
		return B_NO_TRANSLATOR;
	}
	/* look for magic number */
	if (ch[0] != 'P') {
		/* B_TRANSLATOR_BITMAP magic? */
		if (ch[0] == 'b' && ch[1] == 'i') {
			*is_ppm = false;
			return read_bits_header(inSource, 2, width, rowbytes, height, max, ascii, space);
		}
		return B_NO_TRANSLATOR;
	}
	*is_ppm = true;
	if (ch[1] == '6') {
		*ascii = false;
	}
	else if (ch[1] == '3') {
		*ascii = true;
	}
	else {
		return B_NO_TRANSLATOR;
	}
	// status_t err = B_NO_TRANSLATOR;
	enum scan_state {
		scan_width,
		scan_height,
		scan_max,
		scan_white
	} state = scan_width;
	int * scan = NULL;
	bool in_comment = false;
	*space = B_RGB24_BIG;
	/* The description of PPM is slightly ambiguous as far as comments */
	/* go. We choose to allow comments anywhere, in the spirit of laxness. */
	/* See http://www.dcs.ed.ac.uk/~mxr/gfx/2d/PPM.txt */
	int comlen = 0;
	int comptr = 0;
	while (inSource->Read(ch, 1) == 1) {
		if (in_comment && (ch[0] != 10) && (ch[0] != 13)) {
			if (comment) {	/* collect comment(s) into comment string */
				if (comptr >= comlen-1) {
					char * n = (char *)realloc(*comment, comlen+100);
					if (!n) {
						free(*comment);
						*comment = NULL;
					}
					*comment = n;
					comlen += 100;
				}
				(*comment)[comptr++] = ch[0];
				(*comment)[comptr] = 0;
			}
			continue;
		}
		in_comment = false;
		if (ch[0] == '#') {
			in_comment = true;
			continue;
		}
		/* once we're done with whitespace after max, we're done with header */
		if (isdigit(ch[0])) {
			if (!scan) {	/* first digit for this value */
				switch(state) {
				case scan_width:
					scan = width;
					break;
				case scan_height:
					*rowbytes = *width*3;
					scan = height;
					break;
				case scan_max:
					scan = max;
					break;
				default:
					return B_OK;	/* done with header, all OK */
				}
				*scan = 0;
			}
			*scan = (*scan)*10 + (ch[0]-'0');
		}
		else if (isspace(ch[0])) {
			if (scan) {	/* are we done with one value? */
				scan = NULL;
				state = (enum scan_state)(state+1);
			}
			if (state == scan_white) {	/* we only ever read one whitespace, since we skip space */
				return B_OK;	/* when reading ASCII, and there is a single whitespace after max in raw mode */
			}
		}
		else {
			if (state != scan_white) {
				return B_NO_TRANSLATOR;
			}
			return B_OK;	/* header done */
		}
	}
	return B_NO_TRANSLATOR;
}



status_t
read_bits_header(
	BDataIO * io,
	int skipped,
	int * width,
	int * rowbytes,
	int * height, 
	int * max,
	bool * ascii,
	color_space * space)
{
	/* read the rest of a possible B_TRANSLATOR_BITMAP header */
	if (skipped < 0 || skipped > 4) return B_NO_TRANSLATOR;
	int rd = sizeof(TranslatorBitmap)-skipped;
	TranslatorBitmap hdr;
	/* pre-initialize magic because we might have skipped part of it already */
	hdr.magic = B_HOST_TO_BENDIAN_INT32(B_TRANSLATOR_BITMAP);
	char * ptr = (char *)&hdr;
	if (io->Read(ptr+skipped, rd) != rd) {
		return B_NO_TRANSLATOR;
	}
	/* swap header values */
	hdr.magic = B_BENDIAN_TO_HOST_INT32(hdr.magic);
	hdr.bounds.left = B_BENDIAN_TO_HOST_FLOAT(hdr.bounds.left);
	hdr.bounds.right = B_BENDIAN_TO_HOST_FLOAT(hdr.bounds.right);
	hdr.bounds.top = B_BENDIAN_TO_HOST_FLOAT(hdr.bounds.top);
	hdr.bounds.bottom = B_BENDIAN_TO_HOST_FLOAT(hdr.bounds.bottom);
	hdr.rowBytes = B_BENDIAN_TO_HOST_INT32(hdr.rowBytes);
	hdr.colors = (color_space)B_BENDIAN_TO_HOST_INT32(hdr.colors);
	hdr.dataSize = B_BENDIAN_TO_HOST_INT32(hdr.dataSize);
	/* sanity checking */
	if (hdr.magic != B_TRANSLATOR_BITMAP) {
		return B_NO_TRANSLATOR;
	}
	if (hdr.colors & 0xffff0000) {	/* according to <GraphicsDefs.h> this is a reasonable check. */
		return B_NO_TRANSLATOR;
	}
	if (hdr.rowBytes * (hdr.bounds.Height()+1) > hdr.dataSize) {
		return B_NO_TRANSLATOR;
	}
	/* return information about the data in the stream */
	*width = (int)hdr.bounds.Width()+1;
	*rowbytes = hdr.rowBytes;
	*height = (int)hdr.bounds.Height()+1;
	*max = 255;
	*ascii = false;
	*space = hdr.colors;
	return B_OK;
}


/*	String may contain newlines, after which we need to insert extra hash signs. */
status_t
write_comment(
	const char * str,
	BDataIO * io)
{
	const char * end = str+strlen(str);
	const char * ptr = str;
	status_t err = B_OK;
	/* write each line as it's found */
	while ((ptr < end) && !err) {
		if ((*ptr == 10) || (*ptr == 13)) {
			err = io->Write("# ", 2);
			if (err == 2) {
				err = io->Write(str, ptr-str);
				if (err == ptr-str) {
					if (io->Write("\n", 1) == 1) {
						err = 0;
					}
				}
			}
			str = ptr+1;
		}
		ptr++;
	}
	/* write the last data, if any, as a line */
	if (ptr > str) {
		err = io->Write("# ", 2);
		if (err == 2) {
			err = io->Write(str, ptr-str);
			if (err == ptr-str) {
				if (io->Write("\n", 1) == 1) {
					err = 0;
				}
			}
		}
	}
	if (err > 0) {
		err = B_IO_ERROR;
	}
	return err;
}


static status_t
read_ascii_line(
	BDataIO * in,
	int max,
	unsigned char * data,
	int rowbytes)
{
	char ch;
	status_t err;
	// int nread = 0;
	bool comment = false;
	int val = 0;
	bool dig = false;
	while ((err = in->Read(&ch, 1)) == 1) {
		if (comment) {
			if ((ch == '\n') || (ch == '\r')) {
				comment = false;
			}
		}
		if (isdigit(ch)) {
			dig = true;
			val = val * 10 + (ch - '0');
		}
		else {
			if (dig) {
				*(data++) = val*255/max;
				val = 0;
				rowbytes--;
				dig = false;
			}
			if (ch == '#') {
				comment = true;
				continue;
			}
		}
		if (rowbytes < 1) {
			break;
		}
	}
	if (dig) {
		*(data++) = val*255/max;
		val = 0;
		rowbytes--;
		dig = false;
	}
	if (rowbytes < 1) {
		return B_OK;
	}
	return B_IO_ERROR;
}


static status_t
write_ascii_line(
	BDataIO * out,
	unsigned char * data,
	int rowbytes)
{
	char buffer[20];
	int linelen = 0;
	while (rowbytes > 2) {
		sprintf(buffer, "%d %d %d ", data[0], data[1], data[2]);
		rowbytes -= 3;
		int l = strlen(buffer);
		if (l + linelen > 70) {
			out->Write("\n", 1);
			linelen = 0;
		}
		if (out->Write(buffer, l) != l) {
			return B_IO_ERROR;
		}
		linelen += l;
		data += 3;
	}
	out->Write("\n", 1);	/* terminate each scanline */
	return B_OK;
}


static unsigned char * 
make_scale_data(
	int max)
{
	unsigned char * ptr = (unsigned char *)malloc(max);
	for (int ix=0; ix<max; ix++) {
		ptr[ix] = ix*255/max;
	}
	return ptr;
}


static void
scale_data(
	unsigned char * scale,
	unsigned char * data,
	int bytes)
{
	for (int ix=0; ix<bytes; ix++) {
		data[ix] = scale[data[ix]];
	}
}


status_t
copy_data(
	BDataIO * in, 
	BDataIO * out, 
	int rowbytes, 
	int out_rowbytes,
	int height, 
	int max,
	bool in_ascii, 
	bool out_ascii,
	color_space in_space,
	color_space out_space)
{
	dprintf(("copy_data(%x, %x, %x, %x, %x, %x)\n", rowbytes, out_rowbytes, height, max, in_space, out_space));
	/*	We read/write one scanline at a time.	*/
	unsigned char * data = (unsigned char *)malloc(rowbytes);
	unsigned char * out_data = (unsigned char *)malloc(out_rowbytes);
	if (data == NULL || out_data == NULL) {
		free(data);
		free(out_data);
		return B_NO_MEMORY;
	}
	unsigned char * scale = NULL;
	if (max != 255) {
		scale = make_scale_data(max);
	}
	status_t err = B_OK;
	/*	There is no data format conversion, so we can just copy data.	*/
	while ((height-- > 0) && !err) {
		if (in_ascii) {
			err = read_ascii_line(in, max, data, rowbytes);
		}
		else {
			err = in->Read(data, rowbytes);
			if (err == rowbytes) {
				err = B_OK;
			}
			if (scale) {	/* for reading PPM that is smaller than 8 bit */
				scale_data(scale, data, rowbytes);
			}
		}
		if (err == B_OK) {
			unsigned char * wbuf = data;
			if (in_space != out_space) {
				err = convert_space(in_space, out_space, data, rowbytes, out_data);
				wbuf = out_data;
			}
			if (!err && out_ascii) {
				err = write_ascii_line(out, wbuf, out_rowbytes);
			}
			else if (!err) {
				err = out->Write(wbuf, out_rowbytes);
				if (err == out_rowbytes) {
					err = B_OK;
				}
			}
		}
	}
	free(data);
	free(out_data);
	free(scale);
	return err > 0 ? B_IO_ERROR : err;
}


