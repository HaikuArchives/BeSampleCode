#include "TranslatorTemplate.h"
#include <TranslationKit.h>
#include <translation/TranslatorAddOn.h>
#include <app/Application.h>
#include <interface/Alert.h>
#include <interface/StringView.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// !!! Set these five accordingly
#define NATIVE_TRANSLATOR_ACRONYM "MNG"
#define NATIVE_TRANSLATOR_FORMAT 'MNG '
#define NATIVE_TRANSLATOR_MIME_STRING "image/mng"
#define NATIVE_TRANSLATOR_DESCRIPTION "MNG image"
#define copyright_string "© 1999 Be Incorporated"

// The translation kit's native file type
#define B_TRANSLATOR_BITMAP_MIME_STRING "image/x-be-bitmap"
#define B_TRANSLATOR_BITMAP_DESCRIPTION "Be Bitmap image"

// Translation Kit required globals
char translatorName[32];
char translatorInfo[100];
int32 translatorVersion = B_BEOS_VERSION;

// A couple other useful variables
char native_translator_file_name[32];
char native_translator_window_title[32];

static int debug = 0;

status_t CopyInToOut(BPositionIO *in, BPositionIO *out);
status_t TranslatorBitmapToNativeBitmap(BPositionIO *in, BPositionIO *out);
status_t NativeBitmapToTranslatorBitmap(BPositionIO *in, BPositionIO *out);

// Initialize the above
class InitTranslator {
	public:
		InitTranslator() {
			sprintf(translatorName, "%s Images", NATIVE_TRANSLATOR_ACRONYM);
			sprintf(translatorInfo, "%s image translator v%d.%d.%d, %s", NATIVE_TRANSLATOR_ACRONYM,
				(int)(translatorVersion >> 8), (int)((translatorVersion >> 4) & 0xf),
				(int)(translatorVersion & 0xf), __DATE__);
			sprintf(native_translator_file_name, "%sTranslator", NATIVE_TRANSLATOR_ACRONYM);
			sprintf(native_translator_window_title, "%s Settings", NATIVE_TRANSLATOR_ACRONYM);
		}
};
	
static InitTranslator it;

// Define the formats we know how to read
translation_format inputFormats[] = {
	{ NATIVE_TRANSLATOR_FORMAT, B_TRANSLATOR_BITMAP, 0.5, 0.5,
		NATIVE_TRANSLATOR_MIME_STRING, NATIVE_TRANSLATOR_DESCRIPTION },
	{ B_TRANSLATOR_BITMAP, B_TRANSLATOR_BITMAP, 0.5, 0.5,
		B_TRANSLATOR_BITMAP_MIME_STRING, B_TRANSLATOR_BITMAP_DESCRIPTION },
	{ 0, 0, 0, 0, 0, 0 },
};

// Define the formats we know how to write
translation_format outputFormats[] = {
	{ NATIVE_TRANSLATOR_FORMAT, B_TRANSLATOR_BITMAP, 0.5, 0.5,
		NATIVE_TRANSLATOR_MIME_STRING, NATIVE_TRANSLATOR_DESCRIPTION },
	{ B_TRANSLATOR_BITMAP, B_TRANSLATOR_BITMAP, 0.5, 0.5,
		B_TRANSLATOR_BITMAP_MIME_STRING, B_TRANSLATOR_BITMAP_DESCRIPTION },
	{ 0, 0, 0, 0, 0, 0 },
};

// Try to add a configuration view, if it doesn't exist display a message and exit
TranslatorWindow::TranslatorWindow(BRect rect, const char *name) :
	BWindow(rect, name, B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE) {

	BRect extent(0, 0, 239, 239);
	BView *config = NULL;
	status_t err = MakeConfig(NULL, &config, &extent);
	if ((err < B_OK) || (config == NULL)) {
		char error_message[255];
		sprintf(error_message, "%s does not currently allow user configuration.", native_translator_file_name);
		BAlert *alert = new BAlert("No Config", error_message, "Quit");
		alert->Go();
		exit(1);
	}
	
	ResizeTo(extent.Width(), extent.Height());
	AddChild(config);
}

// We're the only window so quit the app too
bool TranslatorWindow::QuitRequested() {
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

// Build the configuration view, making it font sensitive
TranslatorView::TranslatorView(BRect rect, const char *name) :
	BView(rect, name, B_FOLLOW_ALL, B_WILL_DRAW) {

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	font_height fh;
	be_bold_font->GetHeight(&fh);
	BRect r(10, 15, 10 + be_bold_font->StringWidth(translatorName), 15 + fh.ascent + fh.descent);
	
	BStringView *title = new BStringView(r, "Title", translatorName);
	title->SetFont(be_bold_font);
	AddChild(title);
	
	char version_string[100];
	sprintf(version_string, "v%d.%d.%d %s", (int)(translatorVersion >> 8), (int)((translatorVersion >> 4) & 0xf),
		(int)(translatorVersion & 0xf), __DATE__);
	r.top = r.bottom + 20;
	be_plain_font->GetHeight(&fh);
	r.bottom = r.top + fh.ascent + fh.descent;
	r.right = r.left + be_plain_font->StringWidth(version_string);
	
	BStringView *version = new BStringView(r, "Version", version_string);
	version->SetFont(be_plain_font);
	AddChild(version);
	
	r.top = r.bottom + 10;
	r.bottom = r.top + fh.ascent + fh.descent;
	r.right = r.left + be_plain_font->StringWidth(copyright_string);
	
	BStringView *copyright = new BStringView(r, "Copyright", copyright_string);
	copyright->SetFont(be_plain_font);
	AddChild(copyright);
	
	// !!! Add your controls here
}

// Application entry point
int main() {
	char app_signature[255];
	sprintf(app_signature, "application/x-vnd.Be-%s", native_translator_file_name);
	BApplication app(app_signature);
	
	BRect window_rect(100, 100, 339, 339);
	TranslatorWindow *window = new TranslatorWindow(window_rect, native_translator_window_title);
	window->Show();
	
	app.Run();
	return 0;
}

// Determine whether or not we can handle this data
status_t Identify(BPositionIO *inSource, const translation_format *inFormat, BMessage *ioExtension,
	translator_info *outInfo, uint32 outType) {

	if ((outType != 0) && (outType != B_TRANSLATOR_BITMAP) && (outType != NATIVE_TRANSLATOR_FORMAT)) {
		if (debug) printf("Identify(): outType %x is unknown\n", (int)outType);
		return B_NO_TRANSLATOR;
	}
	
	// !!! You might need to make this buffer bigger to test for your native format
	char header[sizeof(TranslatorBitmap)];
	status_t err = inSource->Read(header, sizeof(TranslatorBitmap));
	if (err < B_OK) return err;
	
	if (B_BENDIAN_TO_HOST_INT32(((TranslatorBitmap *)header)->magic) == B_TRANSLATOR_BITMAP) {
		if (debug) printf("Identify(): Found a translator bitmap\n");
		outInfo->type = inputFormats[1].type;
		outInfo->translator = 0;
		outInfo->group = inputFormats[1].group;
		outInfo->quality = inputFormats[1].quality;
		outInfo->capability = inputFormats[1].capability;
		strcpy(outInfo->name, inputFormats[1].name);
		strcpy(outInfo->MIME, inputFormats[1].MIME);
	} else if (0) {	// !!! Change the if statement to look for your format's specific information
		if (debug) printf("Identify(): Found native bitmap\n");
		outInfo->type = inputFormats[0].type;
		outInfo->translator = 0;
		outInfo->group = inputFormats[0].group;
		outInfo->quality = inputFormats[0].quality;
		outInfo->capability = inputFormats[0].capability;
		strcpy(outInfo->name, inputFormats[0].name);
		strcpy(outInfo->MIME, inputFormats[0].MIME);
	} else {
		if (debug) printf("Identify(): Did not find a known magic number or header\n");
		return B_NO_TRANSLATOR;
	}
	
	return B_OK;
}

// Arguably the most important method in the add-on
status_t Translate(BPositionIO *inSource, const translator_info *inInfo, BMessage *ioExtension,
	uint32 outType, BPositionIO *outDestination) {

	// If no specific type was requested, convert to the interchange format
	if (outType == 0) outType = B_TRANSLATOR_BITMAP;
	
	// What action to take, based on the findings of Identify()
	if (outType == inInfo->type) {
		return CopyInToOut(inSource, outDestination);
	} else if (inInfo->type == B_TRANSLATOR_BITMAP && outType == NATIVE_TRANSLATOR_FORMAT) {
		return TranslatorBitmapToNativeBitmap(inSource, outDestination);
	} else if (inInfo->type == NATIVE_TRANSLATOR_FORMAT && outType == B_TRANSLATOR_BITMAP) {
		return NativeBitmapToTranslatorBitmap(inSource, outDestination);
	}

	return B_NO_TRANSLATOR;
}

// Hook to create and return our configuration view
status_t MakeConfig(BMessage *ioExtension, BView **outView, BRect *outExtent) {
	outExtent->Set(0, 0, 239, 239);
	*outView = new TranslatorView(*outExtent, "TranslatorView");
	return B_OK;
}

// The user has requested the same format for input and output, so just copy
status_t CopyInToOut(BPositionIO *in, BPositionIO *out) {
	int block_size = 65536;
	void *buffer = malloc(block_size);
	char temp[1024];
	if (buffer == NULL) {
		buffer = temp;
		block_size = 1024;
	}
	status_t err = B_OK;
	
	// Read until end of file or error
	while (1) {
		ssize_t to_read = block_size;
		err = in->Read(buffer, to_read);
		// Explicit check for EOF
		if (err == -1) {
			if (buffer != temp) free(buffer);
			return B_OK;
		}
		if (err <= B_OK) break;
		to_read = err;
		if (debug) printf("Wrote %d\n", (int)to_read);
		err = out->Write(buffer, to_read);
		if (err != to_read) if (err >= 0) err = B_DEVICE_FULL;
		if (err < B_OK) break;
	}
	
	if (buffer != temp) free(buffer);
	return (err >= 0) ? B_OK : err;
}

// Encode into the native format
status_t TranslatorBitmapToNativeBitmap(BPositionIO *in, BPositionIO *out) {
	TranslatorBitmap header;
	status_t err = in->Read(&header, sizeof(TranslatorBitmap));
	if (err < B_OK) return err;
	else if (err < (int)sizeof(TranslatorBitmap)) return B_ERROR;
	
	// Grab dimension, color space, and size information from the stream
	BRect bounds;
	bounds.left = B_BENDIAN_TO_HOST_FLOAT(header.bounds.left);
	bounds.top = B_BENDIAN_TO_HOST_FLOAT(header.bounds.top);
	bounds.right = B_BENDIAN_TO_HOST_FLOAT(header.bounds.right);
	bounds.bottom = B_BENDIAN_TO_HOST_FLOAT(header.bounds.bottom);
	
	int width = bounds.IntegerWidth() + 1;
	int height = bounds.IntegerHeight() + 1;
	
	color_space cs = (color_space)B_BENDIAN_TO_HOST_INT32(header.colors);
	int row_bytes = B_BENDIAN_TO_HOST_INT32(header.rowBytes);
	
	unsigned char *scanline = (unsigned char *)malloc(row_bytes);
	if (scanline == NULL) return B_NO_MEMORY;
	
	for (int y = 0; y < height; y++) {
		err = in->Read(scanline, row_bytes);
		if (err < row_bytes) {
			free(scanline);
			return (err < B_OK) ? err : B_ERROR;
		}
		
		// !!! Do your conversion here, either scanline by scanline or
		// pixel by pixel, as your format requires
		//
		//for (int x = 0; x < width; x++) {
		//
		//}
		
		// !!! Write out your native data here
	}
	
	free(scanline);
	return B_OK;
}

// Decode the native format
status_t NativeBitmapToTranslatorBitmap(BPositionIO *in, BPositionIO *out) {
	// !!! Initialize this bounds rect to the size of your image
	BRect bounds;
	
	int width = bounds.IntegerWidth() + 1;
	int height = bounds.IntegerHeight() + 1;
	int row_bytes = (bounds.IntegerWidth() + 1) * 4;
	
	// Fill out the B_TRANSLATOR_BITMAP's header
	TranslatorBitmap header;
	header.magic = B_HOST_TO_BENDIAN_INT32(B_TRANSLATOR_BITMAP);
	header.bounds.left = B_HOST_TO_BENDIAN_FLOAT(bounds.left);
	header.bounds.top = B_HOST_TO_BENDIAN_FLOAT(bounds.top);
	header.bounds.right = B_HOST_TO_BENDIAN_FLOAT(bounds.right);
	header.bounds.bottom = B_HOST_TO_BENDIAN_FLOAT(bounds.bottom);
	header.colors = (color_space)B_HOST_TO_BENDIAN_INT32(B_RGB32);
	header.rowBytes = B_HOST_TO_BENDIAN_INT32(row_bytes);
	header.dataSize = B_HOST_TO_BENDIAN_INT32(row_bytes * (bounds.IntegerHeight() + 1));
	
	// Write out the header
	status_t err = out->Write(&header, sizeof(TranslatorBitmap));
	if (err < B_OK) return err;
	else if (err < (int)sizeof(TranslatorBitmap)) return B_ERROR;

	unsigned char *scanline = (unsigned char *)malloc(B_BENDIAN_TO_HOST_INT32(header.rowBytes));
	if (scanline == NULL) return B_NO_MEMORY;
	
	for (int y = 0; y < height; y++) {
		// !!! Get you image data here, do whatever conversions are necessary,
		// and write the data as B,G,R,A to the scanline buffer
		
		// Write the scanline buffer to the output stream
		err = out->Write(scanline, row_bytes);
		if (err < row_bytes) {
			free(scanline);
			return (err < B_OK) ? err : B_ERROR;
		}
	}

	free(scanline);
	return B_OK;
}
