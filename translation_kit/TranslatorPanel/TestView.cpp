#include "TestView.h"
#include "Common.h"
#include <translation/TranslationUtils.h>
#include <translation/TranslatorRoster.h>
#include <translation/BitmapStream.h>
#include <storage/Entry.h>
#include <storage/Path.h>
#include <storage/Directory.h>
#include <storage/File.h>
#include <storage/Node.h>
#include <storage/NodeInfo.h>
#include <interface/MenuBar.h>
#include <interface/Alert.h>
#include <interface/Font.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

TestView::TestView(BRect rect, const char *name) :
	BView(rect, name, B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS) {
	
	bitmap = NULL;
	filters = NULL;
	message_from_tracker = NULL;
	drag_from_tracker = false;
	drag_to_tracker = false;
	messagerunner = NULL;
	SetViewColor(255, 255, 255, 255);
	SetHighColor(255, 255, 255, 255);
}

void TestView::Draw(BRect rect) {
	// Push and pop the drawing state so these changes to the view
	// are only local - this is needed since SetImage() does some drawing
	PushState();
	
	if (HasImage()) {
		DrawBitmap(bitmap, BPoint(0, 0));
	} else {
		BFont font(be_plain_font);
		font.SetSize(16.0);
		SetHighColor(180, 180, 180);
		SetFont(&font);
		
		font_height fh;
		font.GetHeight(&fh);
		const char string[] = "Drop an image here";
		float width = font.StringWidth(string);
		BRect bounds = Bounds();
		DrawString(string, BPoint((bounds.Width() - width) / 2,
			bounds.Height() / 2 + fh.ascent / 2));
		
		if (!drag_from_tracker) {
			SetHighColor(255, 255, 255);
			SetPenSize(2.0);
			bounds.right--;
			bounds.bottom--;
			StrokeRect(bounds);
		}
	}
	
	if (drag_from_tracker) {
		SetHighColor(0, 0, 229);
		SetPenSize(2.0);
		BRect bounds = Bounds();
		bounds.right--;
		bounds.bottom--;
		StrokeRect(bounds);
	}
	
	PopState();
}

// If there's a bitmap, just draw it. If not, invalidate the whole view
// so the background color will be redrawn and the centered text will work
void TestView::FrameResized(float width, float height) {
	if (HasImage()) Draw(Bounds());
	else Invalidate();
}

void TestView::MouseMoved(BPoint point, uint32 state, const BMessage *message) {
	if (drag_to_tracker) return;
	
	if (state == B_EXITED_VIEW) {
		drag_from_tracker = false;
		message_from_tracker = NULL;
		Draw(Bounds());
		return;
	}
	
	if (message == NULL) return;
	if (message->what != B_SIMPLE_DATA) return;
	if (message != message_from_tracker) {
		message_from_tracker = message;
		entry_ref ref;
		if (message->FindRef("refs", 0, &ref) != B_OK) return;
		BNode node(&ref);
		if (node.InitCheck() != B_OK) return;
		BNodeInfo nodeinfo(&node);
		if (nodeinfo.InitCheck() != B_OK) return;
		char mime_type[B_MIME_TYPE_LENGTH];
		if (nodeinfo.GetType(mime_type) != B_OK) return;
		if (strstr(mime_type, "image/") != NULL) {
			drag_from_tracker = true;
			Draw(Bounds());
		}
	}
}

// Set a one time message runner to wake us up and do the drag to Tracker,
// so that casual mouse downs will not initiate the action. This is much
// better than hogging the window's thread by sleeping here
void TestView::MouseDown(BPoint point) {
	drag_to_tracker = true;
	BMessage *message = new BMessage(DRAG_TO_TRACKER);
	message->AddPoint("point", point);
	messagerunner = new BMessageRunner(BMessenger(this), message, 250000, 1);
}

// Turn off drag to tracker - try to beat the DRAG_TO_TRACKER message
void TestView::MouseUp(BPoint point) {
	drag_to_tracker = false;
}

void TestView::MessageReceived(BMessage *message) {
	switch (message->what) {
		case B_COPY_TARGET:
			GetTrackerResponse(message);
			break;
		case DRAG_TO_TRACKER: {
			BPoint point;
			if (message->FindPoint("point", &point) != B_OK) break;
			DragToTracker(point);
			break;
		}
		default:
			BView::MessageReceived(message);
			break;
	}
}

// If the DRAG_TO_TRACKER message arrived before a mouse up disabled
// the action, go ahead and build the transparent bitmap for the drag message
void TestView::DragToTracker(BPoint point) {
	delete messagerunner;
	messagerunner = NULL;
	if (!drag_to_tracker) return;
	drag_to_tracker = false;
	
	if (bitmap == NULL) return;
	if (bitmap->ColorSpace() != B_RGB32 && bitmap->ColorSpace() != B_RGBA32) return;
	BBitmap *drag_bitmap = new BBitmap(bitmap->Bounds(), B_RGBA32);
	if (drag_bitmap == NULL) return;
	uchar *bits = (uchar *)drag_bitmap->Bits();
	if (bits == NULL) {
		delete drag_bitmap;
		return;
	}
	
	int height = bitmap->Bounds().IntegerHeight() + 1;
	int row_bytes = bitmap->BytesPerRow();
	memcpy(bits, bitmap->Bits(), row_bytes * height);
	int area = height * (bitmap->Bounds().IntegerWidth() + 1);
	for (int x = 0; x < area; x++) {
		bits[3] = 0x7f;
		bits += 4;
	}
	
	BMessage message(B_SIMPLE_DATA);
	message.AddInt32("be:actions", B_COPY_TARGET);
	message.AddString("be:filetypes", "application/octet-stream");
	message.AddString("be:types", "application/octet-stream");
	message.AddString("be:clip_name", "Bitmap");
	DragMessage(&message, drag_bitmap, B_OP_BLEND, point);
}

// Find the location and name of the drop and write the image there
void TestView::GetTrackerResponse(BMessage *message) {
	entry_ref dir_ref;
	if (message->FindRef("directory", &dir_ref) != B_OK) return;
	const char *filename;
	if (message->FindString("name", &filename) != B_OK) return;
	BDirectory dir(&dir_ref);
	BFile file(&dir, filename, B_WRITE_ONLY | B_CREATE_FILE);
	if (file.InitCheck() != B_OK) return;
	BBitmapStream stream(bitmap);
	BTranslatorRoster *roster = BTranslatorRoster::Default();
	if (roster->Translate(&stream, NULL, NULL, &file, 'TGA ', 0) != B_OK) {
		BAlert *alert = new BAlert(NULL, "Could not drop image to Tracker.", "OK");
		alert->Go();
	}
	stream.DetachBitmap(&bitmap);
}

void TestView::SetImage(BMessage *message) {
	drag_from_tracker = false;
	entry_ref ref;
	if (message->FindRef("refs", &ref) != B_OK) return;
	BEntry entry(&ref);
	BPath path(&entry);
	BBitmap *new_bitmap = BTranslationUtils::GetBitmapFile(path.Path());
	
	// Only replace the old image and resize the window if the new image
	// was loaded sucessfully
	if (new_bitmap != NULL) {
		if (bitmap != NULL) delete bitmap;
		bitmap = new_bitmap;
		BMenuBar *menubar = Window()->KeyMenuBar();
		Window()->SetSizeLimits(50, 10000, 50, 10000);
		Window()->ResizeTo(bitmap->Bounds().Width(), bitmap->Bounds().Height() +
			menubar->Bounds().Height() + 1);
		// Clear out the old gunk
		FillRect(Bounds());
		// Draw the new image
		Draw(Bounds());
	} else {
		BAlert *alert = new BAlert(NULL, "Could not load the image.", "OK");
		alert->Go();
	}
}

void TestView::SaveImage(BMessage *message) {
	// Recover the necessary data from the message
	translator_id *id;
	uint32 format;
	ssize_t length = sizeof(translator_id);
	if (message->FindData("translator_id", B_RAW_TYPE, (const void **)&id, &length) != B_OK) return;
	if (message->FindInt32("translator_format", (int32 *)&format) != B_OK) return;
	entry_ref dir;
	if (message->FindRef("directory", &dir) != B_OK) return;
	BDirectory bdir(&dir);
	const char *name;
	if (message->FindString("name", &name) != B_OK) return;
	if (name == NULL) return;
	
	// Clobber any existing file or create a new one if it didn't exist
	BFile file(&bdir, name, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (file.InitCheck() != B_OK) {
		BAlert *alert = new BAlert(NULL, "Could not create file.", "OK");
		alert->Go();
		return;
	}
	
	BTranslatorRoster *roster = BTranslatorRoster::Default();
	BBitmapStream stream(bitmap);
	
	// If the id is no longer valid or the translator fails for any other
	// reason, catch it here
	if (roster->Translate(*id, &stream, NULL, &file, format) != B_OK) {
		BAlert *alert = new BAlert(NULL, "Could not save the image.", "OK");
		alert->Go();
	}
	
	// Reclaim the ownership of the bitmap, otherwise it would be deleted
	// when stream passes out of scope
	stream.DetachBitmap(&bitmap);
}

bool TestView::HasImage() {
	if (bitmap != NULL) return true;
	else return false;
}

void TestView::AddFilter(ImageFilter *filter) {
	if (filters == NULL) filters = filter;
	else {
		ImageFilter *temp = filters;
		while (temp->next != NULL) temp = temp->next;
		temp->next = filter;
	}
}

void TestView::FilterImage(BMessage *message) {
	uint32 id;
	if (message->FindInt32("filter_id", (int32 *)&id) != B_OK) return;
	ImageFilter *temp = filters;
	while (true) {
		if (temp == NULL) return;
		if (temp->GetId() == id) break;
		temp = temp->next;
	}
	BBitmap *result = temp->Run(bitmap);
	if (result == NULL) {
		BAlert *alert = new BAlert(NULL, "Error filtering image\n", "OK");
		alert->Go();
		return;
	}
	delete bitmap;
	bitmap = result;
	Invalidate();
}

TestView::~TestView() {
	if (messagerunner != NULL) delete messagerunner;
	if (bitmap != NULL) delete bitmap;
	ImageFilter *temp;
	while (filters != NULL) {
		temp = filters->next;
		delete filters;
		filters = temp;
	}
}