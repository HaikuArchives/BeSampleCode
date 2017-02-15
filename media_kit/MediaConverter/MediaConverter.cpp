// MediaConverter.cpp
//
//   Copyright 1999, Be Incorporated.   All Rights Reserved.
//   This file may be used under the terms of the Be Sample Code License.

#include <Alert.h>
#include <Box.h>
#include <Button.h>
#include <Directory.h>
#include <Entry.h>
#include <ListItem.h>
#include <MediaFile.h>
#include <MediaDefs.h>
#include <MediaFormats.h>
#include <MediaTrack.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h> // for MAX() macro

#include "MediaConverter.h"

const char APP_SIGNATURE[]		= "application/x-vnd.Be.MediaConverter";
const char SOURCE_BOX_LABEL[]	= "Source files";
const char INFO_BOX_LABEL[]		= "File details";
const char OUTPUT_BOX_LABEL[]	= "Output format";
const char FORMAT_LABEL[] 		= "File format";
const char VIDEO_LABEL[] 		= "Video encoding";
const char AUDIO_LABEL[] 		= "Audio encoding";
const char FORMAT_MENU_LABEL[] 	= "Format";
const char VIDEO_MENU_LABEL[] 	= "Video";
const char AUDIO_MENU_LABEL[] 	= "Audio";
const char DURATION_LABEL[]		= "Duration:";
const char VIDEO_INFO_LABEL[]	= "Video:";
const char AUDIO_INFO_LABEL[]	= "Audio:";
const char CONVERT_LABEL[]		= "Convert";
const char CANCEL_LABEL[]		= "Cancel";

const uint32 CONVERT_BUTTON_MESSAGE		= 'cVTB';
const uint32 FORMAT_SELECT_MESSAGE		= 'fMTS';
const uint32 AUDIO_CODEC_SELECT_MESSAGE	= 'aCSL';
const uint32 VIDEO_CODEC_SELECT_MESSAGE	= 'vCSL';
const uint32 FILE_LIST_CHANGE_MESSAGE	= 'fLCH';
const uint32 START_CONVERSION_MESSAGE	= 'stCV';
const uint32 CANCEL_CONVERSION_MESSAGE	= 'cNCV';
const uint32 CONVERSION_DONE_MESSAGE	= 'cVSD';


// ------------------- MediaFileListItem -------------------

class MediaFileListItem : public BStringItem
{
public:
				MediaFileListItem(BMediaFile *f, entry_ref *ref);
	virtual		~MediaFileListItem();

	entry_ref	fRef;
	BMediaFile	*fMediaFile;
};

MediaFileListItem::MediaFileListItem(BMediaFile *f, entry_ref *ref)
	: BStringItem(ref->name),
	  fRef(*ref),
	  fMediaFile(f)
{
}

MediaFileListItem::~MediaFileListItem()
{
	delete fMediaFile;
}

// ------------------- FileFormatMenuItem -------------------

class FileFormatMenuItem : public BMenuItem
{
public:
				FileFormatMenuItem(media_file_format *format);
	virtual		~FileFormatMenuItem();
	
	media_file_format fFileFormat;
};

FileFormatMenuItem::FileFormatMenuItem(media_file_format *format)
	: BMenuItem(format->pretty_name, new BMessage(FORMAT_SELECT_MESSAGE))
{
	memcpy(&fFileFormat, format, sizeof(fFileFormat));
}


FileFormatMenuItem::~FileFormatMenuItem()
{
}

// ------------------- CodecMenuItem -------------------

class CodecMenuItem : public BMenuItem
{
public:
				CodecMenuItem(media_codec_info *ci, uint32 msg_type);
	virtual		~CodecMenuItem();
	
	media_codec_info fCodecInfo;
};

CodecMenuItem::CodecMenuItem(media_codec_info *ci, uint32 msg_type)
	: BMenuItem(ci->pretty_name, new BMessage(msg_type))
{
	memcpy(&fCodecInfo, ci, sizeof(fCodecInfo));
}


CodecMenuItem::~CodecMenuItem()
{
}

// ------------------- MediaFileInfoView implementation -------------------

MediaFileInfoView::MediaFileInfoView(BRect frame, uint32 resizingMode)
	: BView(frame, "MediaFileInfoView", resizingMode,
			B_FULL_UPDATE_ON_RESIZE | B_WILL_DRAW)
{
	fMediaFile = NULL;
	memset(&fRef, 0, sizeof(fRef));
	BFont font;
	GetFont(&font);
	font.SetSize(12);
	SetFont(&font);
}


MediaFileInfoView::~MediaFileInfoView()
{
}

void 
MediaFileInfoView::Update(BMediaFile *f, entry_ref *ref)
{
	fMediaFile = f;
	if (f != NULL) {
		fRef = *ref;
	}
	Invalidate();
}

void 
MediaFileInfoView::GetFileInfo(BString *audioFormat, BString *videoFormat,
							   BString *audioDetails, BString *videoDetails,
							   BString *duration)
{
	if (fMediaFile == NULL) {
		return;
	}
	
	BMediaTrack *track;
	media_format format;
	memset(&format, 0, sizeof(format));
	media_codec_info codecInfo;
	bool audioDone(false), videoDone(false);
	bigtime_t audioDuration(0), videoDuration(0);
	int32 tracks = fMediaFile->CountTracks();
	for (int32 i = 0; i < tracks && (!audioDone || !videoDone); i++) {
		track = fMediaFile->TrackAt(i);
		if (track != NULL) {
			track->EncodedFormat(&format);
			if (format.IsVideo()) {
				memset(&format, 0, sizeof(format));
				format.type = B_MEDIA_RAW_VIDEO;
				track->DecodedFormat(&format);
				media_raw_video_format *rvf = &(format.u.raw_video);
				*videoDetails << (int32)format.Width() << "x" << (int32)format.Height()
							 << " " << (int32)(rvf->field_rate / rvf->interlace)
							 << " fps";
				track->GetCodecInfo(&codecInfo);
				*videoFormat << codecInfo.pretty_name;
				videoDuration = track->Duration();
				videoDone = true;
			} else if (format.IsAudio()) {
				memset(&format, 0, sizeof(format));
				format.type = B_MEDIA_RAW_AUDIO;
				track->DecodedFormat(&format);
				media_raw_audio_format *raf = &(format.u.raw_audio);
				char bytesPerSample = (char)(raf->format & 0xf);
				if (bytesPerSample == 1) {
					*audioDetails << "8 bit ";
				} else if (bytesPerSample == 2) {
					*audioDetails << "16 bit ";
				} else {
					*audioDetails << bytesPerSample << "byte ";
				}
				*audioDetails << (float)(raf->frame_rate / 1000.0f) << " kHz";
				if (raf->channel_count == 1) {
					*audioDetails << " stereo";
				} else if (raf->channel_count == 2) {
					*audioDetails << " mono";
				} else {
					*audioDetails << (int32)raf->channel_count << " channel";
				}
				track->GetCodecInfo(&codecInfo);
				*audioFormat << codecInfo.pretty_name;
				audioDuration = track->Duration();
				audioDone = true;
			}
			fMediaFile->ReleaseTrack(track);
		}	
	}
	
	*duration << (int32)(MAX(audioDuration, videoDuration) / 1000000)
			  << " seconds";
}


void 
MediaFileInfoView::Draw(BRect /*update*/)
{
	font_height fh;
	GetFontHeight(&fh);
	BPoint p(2, fh.ascent + fh.leading), p2;
	BFont font;
	GetFont(&font);
	font.SetFace(B_BOLD_FACE);
	font.SetSize(12);
	SetFont(&font);
	
	if (fMediaFile != NULL) {
		BString aFmt, vFmt, aDetails, vDetails, duration;
		GetFileInfo(&aFmt, &vFmt, &aDetails, &vDetails, &duration);
		
		// draw filename
		DrawString(fRef.name, p);
		float lineHeight = fh.ascent + fh.descent + fh.leading;
		p.y += (float)ceil(lineHeight * 1.5);
		
		float durLen = StringWidth(DURATION_LABEL);
		float audLen = StringWidth(AUDIO_INFO_LABEL);
		float vidLen = StringWidth(VIDEO_INFO_LABEL);
		float maxLen = MAX(durLen, audLen);
		maxLen = MAX(maxLen, vidLen);
				
		// draw labels
		DrawString(AUDIO_INFO_LABEL, p + BPoint(maxLen - audLen, 0));
		p2 = p;
		p2.x += maxLen + 4;
		p.y += lineHeight * 2;
		DrawString(VIDEO_INFO_LABEL, p + BPoint(maxLen - vidLen, 0));
		p.y += lineHeight * 2;
		DrawString(DURATION_LABEL, p + BPoint(maxLen - durLen, 0));

		// draw audio/video/duration info
		font.SetFace(B_REGULAR_FACE);
		font.SetSize(10);
		SetFont(&font);
		
		DrawString(aFmt.String(), p2);
		p2.y += lineHeight;
		DrawString(aDetails.String(), p2);
		p2.y += lineHeight;
		DrawString(vFmt.String(), p2);
		p2.y += lineHeight;
		DrawString(vDetails.String(), p2);
		p2.y += lineHeight;
		DrawString(duration.String(), p2);
	} else {
		DrawString("No file selected", p);
	}
}

void 
MediaFileInfoView::AttachedToWindow()
{
	rgb_color c = Parent()->LowColor();
	SetViewColor(c);
	SetLowColor(c);
}


// ------------------- MediaFileListView implementation -------------------

MediaFileListView::MediaFileListView(BRect frame, uint32 resizingMode)
	: BListView(frame, "MediaFileListView", B_SINGLE_SELECTION_LIST, resizingMode,
				B_WILL_DRAW | B_NAVIGABLE | B_FRAME_EVENTS)
{
	fEnabled = true;
}


MediaFileListView::~MediaFileListView()
{
	BListItem *item;
	while ((item = RemoveItem((int32)0)) != NULL) {
		delete item;
	}
}

void 
MediaFileListView::SetEnabled(bool enabled)
{
	if (enabled != fEnabled) {
		fEnabled = enabled;
	}
}

bool 
MediaFileListView::IsEnabled()
{
	return fEnabled;
}

void
MediaFileListView::AddItem(BMediaFile *f, entry_ref *ref)
{
	BListView::AddItem(new MediaFileListItem(f, ref));
	be_app->PostMessage(FILE_LIST_CHANGE_MESSAGE);
}

void 
MediaFileListView::KeyDown(const char *bytes, int32 numBytes)
{
	switch (bytes[0]) {
	case B_DELETE:
		if (IsEnabled()) {
			int32 selection = CurrentSelection();
			if (selection >= 0) {
				BListItem *item = RemoveItem(selection);
				if (item != NULL) {
					delete item;
				}
				int32 count = CountItems();
				if (selection >= count) {
					selection = count - 1;
				}
				Select(selection);
				be_app->PostMessage(FILE_LIST_CHANGE_MESSAGE);
			}		
		}
		break;
	default:
		BListView::KeyDown(bytes, numBytes);
	}
}

void 
MediaFileListView::SelectionChanged()
{
	MediaConverterWindow *win = dynamic_cast<MediaConverterWindow *>(Window());
	if (win != NULL) {
		win->SourceFileSelectionChanged();
	}
}

// ------------------- StatusView implementation -------------------


StatusView::StatusView(BRect frame, uint32 resizingMode)
	: BView(frame, "StatusView", resizingMode, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
		fStatusString(B_EMPTY_STRING)
{
	frame.OffsetTo(0, 0);
	fStatusRect = frame;
	BFont font;
	GetFont(&font);
	font.SetSize(12);
	SetFont(&font);
}


StatusView::~StatusView()
{
}

void 
StatusView::AttachedToWindow()
{
	rgb_color c = Parent()->LowColor();
	SetViewColor(c);
	SetLowColor(c);
}


void
StatusView::Draw(BRect /*update*/)
{
	// Placement information should be cached here for better performance
	font_height fh;
	GetFontHeight(&fh);
	DrawString(fStatusString.String(),
			   fStatusRect.LeftBottom() + BPoint(1, -fh.descent));
}

void
StatusView::MessageReceived(BMessage *msg)
{
	BView::MessageReceived(msg);
}

void 
StatusView::SetStatus(const char *text)
{
	fStatusString.SetTo(text);
	Invalidate();
}

const char *
StatusView::Status()
{
	return fStatusString.String();
}

// ------------------- MediaConverterWindow implementation -------------------


MediaConverterWindow::MediaConverterWindow(BRect frame)
	: BWindow(frame, "MediaConverter", B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
			  B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS)
{
	fEnabled = true;
	fConverting = false;
	fCancelling = false;
	
	BRect r(frame);
	r.OffsetTo(0, 0);
	BView *background = new BView(r, NULL, B_FOLLOW_ALL_SIDES, 0);
	rgb_color c = ui_color(B_PANEL_BACKGROUND_COLOR);
	background->SetViewColor(c);
	background->SetLowColor(c);
	r.InsetBy(5, 5);

	BRect r2(r);
	r2.bottom -= 30;
	r2.right = r2.left + 150;
	BBox *box = new BBox(r2, NULL, B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM);
	box->SetLabel(SOURCE_BOX_LABEL);

	BRect r3(r2);
	r3.OffsetTo(0, 0);
	r3.InsetBy(5, 5);
	r3.top += 12;
//	r3.bottom -= B_H_SCROLL_BAR_HEIGHT;
	r3.right -= B_V_SCROLL_BAR_WIDTH;
	fListView = new MediaFileListView(r3, B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM);
	BScrollView *scroller = new BScrollView(NULL, fListView,
					B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM, 0, false, true);
	box->AddChild(scroller);
	background->AddChild(box);
	
	r2.left = r2.right + 5;
	r2.right = r.right - 5;
	r2.bottom = r2.top + 120;
	box = new BBox(r2, NULL, B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM);
	box->SetLabel(INFO_BOX_LABEL);
	
	r3 = r2;
	r3.OffsetTo(0, 0);
	r3.InsetBy(5, 5);
	r3.top += 12;
	fInfoView = new MediaFileInfoView(r3, B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM);
	box->AddChild(fInfoView);
	background->AddChild(box);

	r2.top = r2.bottom + 5;
	r2.bottom = r.bottom - 30;
	box = new BBox(r2, NULL, B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM);
	box->SetLabel(OUTPUT_BOX_LABEL);

	r3 = r2;
	r3.OffsetTo(0, 0);
	r3.InsetBy(5, 5);
	r3.top += 12;
	
	BRect r4(r3);
	r4.bottom = r4.top + 20;
	BPopUpMenu *popmenu = new BPopUpMenu(FORMAT_MENU_LABEL);
	fFormatMenu = new BMenuField(r4, NULL, FORMAT_LABEL, popmenu);
	float maxLabelLen = fFormatMenu->StringWidth(FORMAT_LABEL);
	box->AddChild(fFormatMenu);
	
	r4.top = r4.bottom + 5;
	r4.bottom = r4.top + 20;
	popmenu = new BPopUpMenu(AUDIO_MENU_LABEL);
	fAudioMenu = new BMenuField(r4, NULL, AUDIO_LABEL, popmenu);
	maxLabelLen = MAX(maxLabelLen, fAudioMenu->StringWidth(AUDIO_LABEL));
	box->AddChild(fAudioMenu);

	r4.top = r4.bottom + 5;
	r4.bottom = r4.top + 20;
	popmenu = new BPopUpMenu(VIDEO_MENU_LABEL);
	fVideoMenu = new BMenuField(r4, NULL, VIDEO_LABEL, popmenu);
	maxLabelLen = MAX(maxLabelLen, fVideoMenu->StringWidth(VIDEO_LABEL));
	box->AddChild(fVideoMenu);
	background->AddChild(box);

	maxLabelLen += 5;
	fFormatMenu->SetDivider(maxLabelLen);
	fAudioMenu->SetDivider(maxLabelLen);
	fVideoMenu->SetDivider(maxLabelLen);

	r2.top = r2.bottom + 10;
	r2.bottom = r.bottom;
	r2.left = r.left;
	r2.right = r.right;
	fConvertButton = new BButton(r2, NULL, CONVERT_LABEL,
								 new BMessage(CONVERT_BUTTON_MESSAGE));
	background->AddChild(fConvertButton);
	fConvertButton->ResizeToPreferred();
	BRect buttonFrame(fConvertButton->Frame());
	buttonFrame.OffsetTo(r.right - buttonFrame.Width(),
						 r.bottom - buttonFrame.Height());
	fConvertButton->MoveTo(buttonFrame.LeftTop());
	r2.right = buttonFrame.left - 5;

	fStatusView = new StatusView(r2, B_FOLLOW_LEFT | B_FOLLOW_TOP);
	background->AddChild(fStatusView);

	AddChild(background);
	
	SetStatusMessage("Drop media files onto this window");
}


MediaConverterWindow::~MediaConverterWindow()
{
}

void
MediaConverterWindow::BuildAudioVideoMenus()
{
	BMenu *menu = fAudioMenu->Menu();
	BMenuItem *item;
	// clear out old audio codec menu items
	while ((item = menu->RemoveItem((int32)0)) != NULL) {
		delete item;
	}

	// get selected file format
	FileFormatMenuItem *ffmi = (FileFormatMenuItem*)fFormatMenu->Menu()->FindMarked();
	media_file_format *mf_format = &(ffmi->fFileFormat);

	media_format format, outfmt;
	memset(&format, 0, sizeof(format));
	media_codec_info codec_info;
	int32 cookie = 0;
	CodecMenuItem *cmi;

	// add available audio encoders to menu
	format.type = B_MEDIA_RAW_AUDIO;
	format.u.raw_audio = media_raw_audio_format::wildcard;	
	while (get_next_encoder(&cookie, mf_format, &format, &outfmt, &codec_info) == B_OK) {
		cmi = new CodecMenuItem(&codec_info, AUDIO_CODEC_SELECT_MESSAGE);
		menu->AddItem(cmi);
		// reset media format struct
		format.type = B_MEDIA_RAW_AUDIO;
		format.u.raw_audio = media_raw_audio_format::wildcard;
	}

	// mark first audio encoder
	item = menu->ItemAt(0);
	if (item != NULL) {
		fAudioMenu->SetEnabled(fEnabled);
		item->SetMarked(true);
		((BInvoker *)item)->Invoke();
	} else {
		item = new BMenuItem("None available", NULL);
		menu->AddItem(item);
		item->SetMarked(true);
		fAudioMenu->SetEnabled(false);
	}
	
	// clear out old video codec menu items
	menu = fVideoMenu->Menu();
	while ((item = menu->RemoveItem((int32)0)) != NULL) {
		delete item;
	}

	// construct a generic video format.  Some of these parameters
	// seem silly, but are needed for R4.5.x, which is more picky
	// than subsequent BeOS releases will be.
	memset(&format, 0, sizeof(format));
	format.type = B_MEDIA_RAW_VIDEO;
	format.u.raw_video.last_active = (uint32)(320 - 1);
	format.u.raw_video.orientation = B_VIDEO_TOP_LEFT_RIGHT;
	format.u.raw_video.display.format = B_RGB32;
	format.u.raw_video.display.line_width = (int32)320;
	format.u.raw_video.display.line_count = (int32)240;
	format.u.raw_video.display.bytes_per_row = 4 * 320;

	// add available video encoders to menu
	cookie = 0;
	while (get_next_encoder(&cookie, mf_format, &format, &outfmt, &codec_info) == B_OK) {
		cmi = new CodecMenuItem(&codec_info, VIDEO_CODEC_SELECT_MESSAGE);
		menu->AddItem(cmi);
	}

	// mark first video encoder
	item = menu->ItemAt(0);
	if (item != NULL) {
		fVideoMenu->SetEnabled(fEnabled);
		item->SetMarked(true);
		((BInvoker *)item)->Invoke();
	} else {
		item = new BMenuItem("None available", NULL);
		menu->AddItem(item);
		item->SetMarked(true);
		fVideoMenu->SetEnabled(false);
	}
}

void
MediaConverterWindow::GetSelectedFormatInfo(media_file_format **format,
											media_codec_info **audio,
											media_codec_info **video)
{
	*format = NULL;
	FileFormatMenuItem *formatItem =
		dynamic_cast<FileFormatMenuItem *>(fFormatMenu->Menu()->FindMarked());
	if (formatItem != NULL) {
		*format = &(formatItem->fFileFormat);
	}
	
	*audio = *video = NULL;
	CodecMenuItem *codecItem =
		dynamic_cast<CodecMenuItem *>(fAudioMenu->Menu()->FindMarked());
	if (codecItem != NULL) {
		*audio =  &(codecItem->fCodecInfo);
	}
	codecItem = dynamic_cast<CodecMenuItem *>(fVideoMenu->Menu()->FindMarked());
	if (codecItem != NULL) {
		*video =  &(codecItem->fCodecInfo);
	}
}


void 
MediaConverterWindow::BuildFormatMenu()
{
	BMenu *menu = fFormatMenu->Menu();
	BMenuItem *item;
	// clear out old format menu items
	while ((item = menu->RemoveItem((int32)0)) != NULL) {
		delete item;
	}

	// add menu items for each file format
	media_file_format mfi;
	int32 cookie = 0;
	FileFormatMenuItem *ff_item;
	while (get_next_file_format(&cookie, &mfi) == B_OK) {
		ff_item = new FileFormatMenuItem(&mfi);
		menu->AddItem(ff_item);
	}
	
	// mark first item
	item = menu->ItemAt(0);
	if (item != NULL) {
		item->SetMarked(true);
		((BInvoker *)item)->Invoke();
	}
}

void 
MediaConverterWindow::SetStatusMessage(const char *message)
{
	fStatusView->SetStatus(message);
}

void
MediaConverterWindow::AddSourceFile(BMediaFile *f, entry_ref *ref)
{
	fListView->AddItem(f, ref);
}

void
MediaConverterWindow::RemoveSourceFile(int32 index)
{
	BListItem *item = fListView->RemoveItem(index);
	if (item != NULL) {
		delete item;
	}
}

int32 
MediaConverterWindow::CountSourceFiles()
{
	return fListView->CountItems();
}

status_t
MediaConverterWindow::GetSourceFileAt(int32 index, BMediaFile **f, entry_ref *ref)
{
	MediaFileListItem *item = dynamic_cast<MediaFileListItem*>(fListView->ItemAt(index));
	if (item != NULL) {
		*f = item->fMediaFile;
		*ref = item->fRef;
		return B_OK;
	} else {
		return B_ERROR;
	}
}

void 
MediaConverterWindow::SourceFileSelectionChanged()
{
	int32 selected = fListView->CurrentSelection();
	BMediaFile *f = NULL;
	entry_ref *ref = NULL;
	if (selected >= 0) {
		MediaFileListItem* mfli = dynamic_cast<MediaFileListItem*>(fListView->ItemAt(selected));
		if (mfli != NULL) {
			f = mfli->fMediaFile;
			ref = &(mfli->fRef);
		}
	}
	fInfoView->Update(f, ref);
}

void 
MediaConverterWindow::SetEnabled(bool enabled, bool buttonEnabled)
{
	fConvertButton->SetEnabled(buttonEnabled);
	if (enabled != fEnabled) {
		fFormatMenu->SetEnabled(enabled);
		fAudioMenu->SetEnabled(enabled);
		fVideoMenu->SetEnabled(enabled);
		fListView->SetEnabled(enabled);
		fEnabled = enabled;
	}
}

bool 
MediaConverterWindow::IsEnabled()
{
	return fEnabled;
}

void 
MediaConverterWindow::DispatchMessage(BMessage *msg, BHandler *handler)
{
	if (msg->WasDropped() && msg->what == B_SIMPLE_DATA) {
		DetachCurrentMessage();
		msg->what = B_REFS_RECEIVED;
		be_app->PostMessage(msg);
		delete msg;
	} else {
		BWindow::DispatchMessage(msg, handler);
	}
}


void 
MediaConverterWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
	case FORMAT_SELECT_MESSAGE:
		BuildAudioVideoMenus();
		break;
	case AUDIO_CODEC_SELECT_MESSAGE:
		break;
	case VIDEO_CODEC_SELECT_MESSAGE:
		break;
	case CONVERT_BUTTON_MESSAGE:
		if (!fConverting) {
			fConvertButton->SetLabel(CANCEL_LABEL);
			fConverting = true;
			SetStatusMessage("Converting...");
			SetEnabled(false, true);
			be_app->PostMessage(START_CONVERSION_MESSAGE);
		} else if (!fCancelling) {
			fCancelling = true;
			SetStatusMessage("Cancelling...");
			be_app->PostMessage(CANCEL_CONVERSION_MESSAGE);			
		}
		break;
	case CONVERSION_DONE_MESSAGE:
		SetStatusMessage(fCancelling ? "Conversion cancelled" : "Conversion complete");
		fConverting = false;
		fCancelling = false;
		{
			bool enable = CountSourceFiles() > 0;
			SetEnabled(enable, enable);
		}
		fConvertButton->SetLabel(CONVERT_LABEL);
		break;
	default:
		BWindow::MessageReceived(msg);
	}
}

bool
MediaConverterWindow::QuitRequested()
{
	if (!fConverting) {
		be_app->PostMessage(B_QUIT_REQUESTED);
		return true;
	} else if (!fCancelling) {
		fCancelling = true;
		SetStatusMessage("Cancelling...");
		be_app->PostMessage(CANCEL_CONVERSION_MESSAGE);
	}
	return false;
}

// ------------------- MediaConverterApp implementation -------------------

MediaConverterApp::MediaConverterApp()
	: BApplication(APP_SIGNATURE)
{
	fConverting = false;
	fCancel = false;
	fConvertThreadID = -1;
	fWin = new MediaConverterWindow(BRect(50, 50, 480, 320));
}


MediaConverterApp::~MediaConverterApp()
{
}

BMediaFile*
MediaConverterApp::CreateOutputFile(BMediaFile */*input*/,  entry_ref *ref,
									media_file_format *outputFormat)
{
	BString name(ref->name);
	// create output file name
	int32 extIndex = name.FindLast('.');
	if (extIndex != B_ERROR) {
		name.Truncate(extIndex + 1);
	}
	name.Append(outputFormat->file_extension);

	BDirectory dir;
	BEntry inEntry(ref);
	BEntry outEntry;
	if (inEntry.InitCheck() == B_OK && inEntry.GetParent(&dir) == B_OK) {
		// ensure that output name is unique
		int32 len = name.Length();
		int32 i = 1;
		while (dir.Contains(name.String())) {
			name.Truncate(len);
			name << " " << i;
			i++;
		}
		outEntry.SetTo(&dir, name.String());
	}
	
	BMediaFile *f = NULL;
	if (outEntry.InitCheck() == B_OK) {
		entry_ref outRef;
		outEntry.GetRef(&outRef);
		f = new BMediaFile(&outRef, outputFormat);
		name.Prepend("Output file '");
		name.Append("' created");
	} else {
		name.Prepend("Error creating '");
		name.Append("'");
	}
	
	if (fWin->Lock()) {
		fWin->SetStatusMessage(name.String());
		fWin->Unlock();
	}
	
	return f;
}

bool 
MediaConverterApp::IsConverting()
{
	return fConverting;
}

void 
MediaConverterApp::StartConverting()
{
	bool locked = fWin->Lock();

	if (locked && (fWin->CountSourceFiles() > 0)) {
		fConvertThreadID = spawn_thread(MediaConverterApp::RunConvert, "converter thread",
										B_NORMAL_PRIORITY, (void *)this);
		if (fConvertThreadID > 0) {
			fConverting = true;
			fCancel = false;
			resume_thread(fConvertThreadID);			
		} else {
			fConvertThreadID = -1;
		}
	}

	if (locked) {
		fWin->Unlock();
	}
}

void 
MediaConverterApp::ConvertLoop()
{
	int32 srcIndex = 0;
	while (!fCancel) {
		if (fWin->Lock()) {
			BMediaFile *inFile(NULL), *outFile(NULL);
			entry_ref inRef;
			status_t r = fWin->GetSourceFileAt(srcIndex, &inFile, &inRef);
			if (r == B_OK) {
				media_codec_info *audioCodec, *videoCodec;
				media_file_format *fileFormat;
				fWin->GetSelectedFormatInfo(&fileFormat, &audioCodec, &videoCodec);
				fWin->Unlock();
				outFile = CreateOutputFile(inFile, &inRef, fileFormat);
				if (outFile != NULL) {
					r = ConvertFile(inFile, outFile, audioCodec, videoCodec);
					fWin->Lock();
					if (r == B_OK) {
						fWin->RemoveSourceFile(srcIndex);
					} else {
						srcIndex++;
						BString error("Error converting '");
						error << inRef.name << "'";
						fWin->SetStatusMessage(error.String());
					}
					fWin->Unlock();
				}
			} else {
				fWin->Unlock();
				break;
			}
		} else {
			break;
		}
	}
	
	PostMessage(CONVERSION_DONE_MESSAGE);
}

status_t 
MediaConverterApp::ConvertFile(BMediaFile *inFile, BMediaFile *outFile,
							   media_codec_info *audioCodec,
							   media_codec_info *videoCodec)
{
	BMediaTrack *inVidTrack(NULL), *inAudTrack(NULL);
	BMediaTrack *outVidTrack(NULL), *outAudTrack(NULL);
	media_format inFormat, outAudFormat, outVidFormat;
	media_raw_audio_format *raf(NULL);
	media_raw_video_format *rvf(NULL);
	int32 width(-1), height(-1);
	short audioFrameSize(1);
	char *videoData(NULL);
	char *audioData(NULL);

	// gather the necessary format information and construct output tracks
	int32 tracks = inFile->CountTracks();
	for (int32 i = 0; i < tracks && (!outAudTrack || !outVidTrack); i++) {
		BMediaTrack *inTrack = inFile->TrackAt(i);
		inTrack->EncodedFormat(&inFormat);

		if (inFormat.IsAudio() && (audioCodec != NULL)) {
			inAudTrack = inTrack;
			memcpy(&outAudFormat, &inFormat, sizeof(outAudFormat));
			outAudFormat.type = B_MEDIA_RAW_AUDIO;
			raf = &(outAudFormat.u.raw_audio);
			inTrack->DecodedFormat(&outAudFormat);
			
			audioData = (char*)malloc(raf->buffer_size);
//			audioFrameSize = (raf->format & media_raw_audio_format::B_AUDIO_SIZE_MASK)
			audioFrameSize = (raf->format & 0xf)
								* raf->channel_count;
			outAudTrack = outFile->CreateTrack(&outAudFormat, audioCodec);	
		} else if (inFormat.IsVideo() && (videoCodec != NULL)) {
			inVidTrack = inTrack;
			width = (int32)inFormat.Width();
			height = (int32)inFormat.Height();
			
			// construct desired decoded video format
			memset(&outVidFormat, 0, sizeof(outVidFormat));
			outVidFormat.type = B_MEDIA_RAW_VIDEO;
			rvf = &(outVidFormat.u.raw_video);
			rvf->last_active = (uint32)(height - 1);
			rvf->orientation = B_VIDEO_TOP_LEFT_RIGHT;
			rvf->pixel_width_aspect = 1;
			rvf->pixel_height_aspect = 3;
			rvf->display.format = B_RGB32;
			rvf->display.bytes_per_row = 4 * width;
			rvf->display.line_width = width;
			rvf->display.line_count = height;

			inVidTrack->DecodedFormat(&outVidFormat);
			videoData = (char *)malloc(width * height * 4);
			outVidTrack = outFile->CreateTrack(&outVidFormat, videoCodec);
		} else {
			inFile->ReleaseTrack(inTrack);		
		}
	}
	
	if (fCancel || (!outVidTrack && !outAudTrack)) {
		// don't have any video or audio tracks here, or cancelled
		delete outFile;
		return B_ERROR;
	} else {
		outFile->CommitHeader();
		// this is where you would call outFile->AddCopyright(...)
	}

	int64 frameCount, framesRead;
	status_t err;
	media_header mh;
	int32 lastPercent, currPercent;
	float completePercent;
	BString status;

	// read video from source and write to destination, if necessary
	if (outVidTrack != NULL) {
		frameCount = inVidTrack->CountFrames();
		lastPercent = -1;
		for (int64 i = 0; (i < frameCount) && !fCancel; i++) {
			framesRead = 1;
			if ((err = inVidTrack->ReadFrames(videoData, &framesRead, &mh)) != B_OK) {
				fprintf(stderr, "Error reading video frame %Ld: %s\n", i,
						strerror(err));
				break;
			}
			if ((err = outVidTrack->WriteFrames(videoData, 1,
					mh.u.encoded_video.field_flags)) != B_OK)
			{
				fprintf(stderr, "Error writing video frame %Ld: %s\n", i,
						strerror(err));
				break;
			}
			completePercent = ((float)i) / ((float)frameCount) * 100;
			currPercent = (int16)floor(completePercent);
			if (currPercent > lastPercent) {
				lastPercent = currPercent;
				status.SetTo("Writing video track: ");
				status << currPercent << "% complete";
				SetStatusMessage(status.String());
			}
		}
		outVidTrack->Flush();
		inFile->ReleaseTrack(inVidTrack);
	}
	
	// read audio from source and write to destination, if necessary
	if (outAudTrack != NULL) {
		frameCount = inAudTrack->CountFrames();
		lastPercent = -1;
		for (int64 i = 0; (i < frameCount) && !fCancel; i += framesRead) {
			if ((err = inAudTrack->ReadFrames(audioData, &framesRead, &mh)) != B_OK) {
				fprintf(stderr, "Error reading audio frames: %s\n", strerror(err));
				break;
			}
			if ((err = outAudTrack->WriteFrames(audioData, framesRead)) != B_OK)
			{
				fprintf(stderr, "Error writing audio frames: %s\n", strerror(err));
				break;
			}
			completePercent = ((float)i) / ((float)frameCount) * 100;
			currPercent = (int16)floor(completePercent);
			if (currPercent > lastPercent) {
				lastPercent = currPercent;
				status.SetTo("Writing audio track: ");
				status << currPercent << "% complete";
				SetStatusMessage(status.String());
			}
		}
		outAudTrack->Flush();
		inFile->ReleaseTrack(inAudTrack);
	}

	outFile->CloseFile();
	delete outFile;
	free(videoData);
	free(audioData);
	
	return B_OK;
}


void 
MediaConverterApp::SetStatusMessage(const char *message)
{
	if (fWin != NULL && fWin->Lock()) {
		fWin->SetStatusMessage(message);
		fWin->Unlock();
	}
}


void 
MediaConverterApp::ReadyToRun()
{
	fWin->Show();
	if (fWin->Lock()) {
		fWin->BuildFormatMenu();
		if (fWin->CountSourceFiles() == 0) {
			fWin->SetEnabled(false, false);
		}
		fWin->Unlock();
	}
}

void 
MediaConverterApp::RefsReceived(BMessage *msg)
{
	entry_ref ref;
	int32 i = 0;
	BMediaFile *f;
	BString errorFiles;
	int32 errors = 0;
	while (msg->FindRef("refs", i++, &ref) == B_OK) {
		f = new BMediaFile(&ref/*, B_MEDIA_FILE_NO_READ_AHEAD*/);
		if (f->InitCheck() != B_OK) {
			errorFiles << ref.name << "\n";
			errors++;
			delete f;
			continue;
		}
		if (fWin->Lock()) {
			fWin->AddSourceFile(f, &ref);
			fWin->Unlock();
		}
	}
	
	if (errors) {
		BString alertText;
		alertText << errors << ((errors > 1) ? "files" : "file")
				  << " were not recognized as supported media files:\n";
		alertText << errorFiles;
		BAlert *alert = new BAlert("Error loading file", alertText.String(),
							"Continue", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->Go();
	}
}



void 
MediaConverterApp::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
	case FILE_LIST_CHANGE_MESSAGE:
		if (fWin->Lock()) {
			bool enable = fWin->CountSourceFiles() > 0;
			fWin->SetEnabled(enable, enable);
			fWin->Unlock();
		}
		break;
	case START_CONVERSION_MESSAGE:
		if (!fConverting) {
			StartConverting();			
		}
		break;
	case CANCEL_CONVERSION_MESSAGE:
		fCancel = true;
		break;
	case CONVERSION_DONE_MESSAGE:
		fCancel = false;
		fConverting = false;
		DetachCurrentMessage();
		fWin->PostMessage(msg);
		break;
	default:
		BApplication::MessageReceived(msg);
	}
}

int32 
MediaConverterApp::RunConvert(void *castToMediaConverterApp)
{
	MediaConverterApp *app = (MediaConverterApp *)castToMediaConverterApp;
	app->ConvertLoop();
	return 0;
}


int main(int, char **)
{
	MediaConverterApp app;
	app.Run();
	
	return 0;
}
