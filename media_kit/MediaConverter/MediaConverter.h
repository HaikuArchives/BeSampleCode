// MediaConverter.h
//
//   A demo program that converts audio or video files to a different format.
//   Source files can be in any file format for which there is an installed
//   extractor and decoder.  Destination files can be any file format for which
//   there is a writer and audio/video encoders.  For example, MediaConverter
//   can convert a Quicktime movie with Cinepak encoded video and IMA4 audio
//   to an AVI file with Indeo5 encoded video and raw audio on a standard
//   BeOS R4.5 installation because there proper encoders and decoders are
//   installed by default.
//
//   Copyright 1999, Be Incorporated.   All Rights Reserved.
//   This file may be used under the terms of the Be Sample Code License.

#ifndef MEDIACONVERTER_H
#define MEDIACONVERTER_H

#include <Application.h>
#include <ListView.h>
#include <Rect.h>
#include <String.h>
#include <View.h>
#include <Window.h>

class BMediaFile;
class BMenuField;
struct media_codec_info;
struct media_file_format;

class MediaFileInfoView : public BView
{
public:
				MediaFileInfoView(BRect frame, uint32 resizingMode);
	virtual		~MediaFileInfoView();

	void		Update(BMediaFile *f, entry_ref *ref);
	void		GetFileInfo(BString *audioFormat, BString *videoFormat,
							BString *audioDetails, BString *videoDetails,
							BString *duration);
protected:

	virtual void Draw(BRect update);
	virtual void AttachedToWindow();

private:
	entry_ref	fRef;
	BMediaFile*	fMediaFile;
};

class MediaFileListView : public BListView
{
public:
				MediaFileListView(BRect frame, uint32 resizingMode);
	virtual		~MediaFileListView();

	void		AddItem(BMediaFile *f, entry_ref *ref);

	void		SetEnabled(bool enabled);
	bool		IsEnabled();

protected:

	virtual void KeyDown(const char *bytes, int32 numBytes);
	virtual void SelectionChanged();

private:
	bool		fEnabled;
};


class StatusView : public BView
{
public:
					StatusView(BRect frame, uint32 resizingMode);
	virtual			~StatusView();

	void			SetStatus(const char *text);
	const char*		Status();

protected:

	virtual void	AttachedToWindow();
	virtual void	Draw(BRect update);
	virtual void	MessageReceived(BMessage *msg);

private:
	BRect		fStatusRect;
	BString		fStatusString;
};

class MediaConverterWindow : public BWindow
{
public:

				MediaConverterWindow(BRect frame);
	virtual		~MediaConverterWindow();

	void		BuildFormatMenu();
	void		BuildAudioVideoMenus();
	void		GetSelectedFormatInfo(media_file_format **format,
									 media_codec_info **audio,
									 media_codec_info **video);
	
	void		SetStatusMessage(const char *message);
	
	void		AddSourceFile(BMediaFile *f, entry_ref *ref);
	void		RemoveSourceFile(int32 index);
	int32		CountSourceFiles();
	status_t	GetSourceFileAt(int32 index, BMediaFile **f, entry_ref *ref);

	void		SourceFileSelectionChanged();

	void		SetEnabled(bool enabled, bool buttonEnabled);
	bool		IsEnabled();
	
protected:
	virtual void	DispatchMessage(BMessage *msg, BHandler *handler);
	virtual void	MessageReceived(BMessage *msg);
	virtual bool	QuitRequested();

private:
	media_format	fDummyFormat;
	BButton		*fConvertButton;
	BMenuField	*fFormatMenu;
	BMenuField	*fVideoMenu;
	BMenuField	*fAudioMenu;
	StatusView	*fStatusView;
	MediaFileListView	*fListView;
	MediaFileInfoView	*fInfoView;
	bool	fEnabled;
	bool	fConverting;
	bool	fCancelling;
};

class MediaConverterApp : public BApplication
{
public:
				MediaConverterApp();
	virtual		~MediaConverterApp();

	BMediaFile*	CreateOutputFile(BMediaFile *input, entry_ref *ref,
								 media_file_format *outputFormat);
	
	bool		IsConverting();
	void		StartConverting();
	void		ConvertLoop();
	status_t	ConvertFile(BMediaFile *inFile, BMediaFile *outFile,
							media_codec_info *audioCodec,
							media_codec_info *videoCodec);

	void		SetStatusMessage(const char *message);

protected:
	virtual void	MessageReceived(BMessage *msg);
	virtual void	ReadyToRun();
	virtual void	RefsReceived(BMessage *msg);
	
private:
	static int32	RunConvert(void *castToMediaConverterApp);
	
	MediaConverterWindow *fWin;
	thread_id	fConvertThreadID;
	bool	fConverting;
	bool	fCancel;
};

#endif // MEDIACONVERTER_H