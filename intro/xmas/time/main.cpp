/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <stdio.h>

#include <Bitmap.h>
#include <Screen.h>

#include <string.h>

#include <MediaDefs.h> 
#include <Sound.h> 
#include <SoundPlayer.h> 
#include <Entry.h> 
#include <Path.h> 
#include <Resources.h>
#include <AppFileInfo.h>

#include "UptimeWindow.h"
#include "UptimeView.h"
#include "BitmapView.h"
#include <String.h>
#include <File.h>
#include <TranslationUtils.h>

#include <Application.h>
#include <Roster.h>

typedef	int32	(*thread_func) (void *);

class XmasApplication : public BApplication {

public:
		XmasApplication();
		virtual ~XmasApplication();
		
		static int32 folge_entry(void* obj)
		{
			return ((XmasApplication*)obj)->folge();
		}
		
		int32 folge();
		status_t playsound(const char* path);
		
private:
	thread_id				folgeThread;
	bool					quitting;
	BSoundPlayer*			player;
	sem_id					play_sem;
};



status_t XmasApplication::playsound(const char* path)
{
	status_t err;
	BSoundPlayer::play_id	id;

	BEntry entry(path, true);
	err = entry.InitCheck();	
	if (err != B_OK) {
		fprintf(stderr, "Couldn't find %s.\n", path);
		return err;
	}

	entry_ref ref;
	err = entry.GetRef(&ref);
	if (err != B_OK) {
		fprintf(stderr, "Couldn't get entry_ref from %s.\n", path);
		return err;
	}

	
	BSound* sound = new BSound(&ref);
	err = sound->InitCheck();
	if (err != B_OK) {
		fprintf(stderr, "%s not a valid sound file.\n", path);
		return err;
	}


	err = acquire_sem(play_sem);
	if (err != B_OK)
		return err;
		
	media_raw_audio_format fmt = sound->Format();
	player = new BSoundPlayer(&fmt, ref.name);
	player->Start();
	player->SetVolume(0.1);
	id = player->StartPlaying(sound);
	sound->ReleaseRef();
	
	release_sem_etc(play_sem, 1, B_DO_NOT_RESCHEDULE);
	
	player->WaitForSound(id);
	
	err = acquire_sem(play_sem);
	if (err != B_OK)
		return err;
		
	delete player;
	player = 0;
	
	release_sem(play_sem);
	
	return B_OK;
}

int32 XmasApplication::folge()
{
	app_info ai;
	BFile file;
	BAppFileInfo afi;
	BPath file_path;
	BPath parent;
		
	be_app->GetAppInfo(&ai);
	file.SetTo(&ai.ref, B_READ_WRITE);
	afi.SetTo(&file);
	BEntry entry(&ai.ref, FALSE);
	entry.GetParent(&entry);
	entry.GetPath(&file_path);
	fprintf(stderr, "path %s\n", file_path.Path());

	BString path_string;
	path_string+=file_path.Path();
	path_string+="/xmas.wav";

	while (! quitting)
	{
		if ((playsound(path_string.String())) != B_OK) {
			snooze(100000LL);
		}
	}
	
	return(0);
}



int main() {

	XmasApplication	myApplication;
	
	myApplication.Run();
	
	return(0);
}

XmasApplication::XmasApplication() :

	BApplication("application/x-vnd.Be-xmas")
{
	UptimeWindow	*aWindow;
	UptimeView		*aView;
	BitmapView		*bView;
	BRect			aRect;

	BBitmap *onscreen = BTranslationUtils::GetBitmap("xmas_bmap");
	BRect bRect = onscreen->Bounds();
	
	quitting = false;
	play_sem = create_sem(1, "play_sem");
	player = 0;
	folgeThread = spawn_thread(folge_entry, "Folgen mich!", B_NORMAL_PRIORITY, this);
	resume_thread(folgeThread);

	// set up a rectangle and instantiate a new window
	aRect.Set(0,0, 400, 350);
	bRect.Set(90, 30, 400, 350);

	aWindow = new UptimeWindow(aRect);

	bView = new BitmapView(onscreen, bRect, "BitmapView");
	aView = new UptimeView(aRect, "UptimeView");


	aWindow->Lock();
	
	aWindow->AddChild(aView);
	aView->AddChild(bView);

	aWindow->MoveTo(25,25);

	aWindow->Unlock();
	
	aWindow->Show();


}

XmasApplication::~XmasApplication()
{
	quitting = true;
	if (acquire_sem(play_sem) == B_OK) {
		if (player) {
			player->Stop();
		}
		release_sem(play_sem);
	}
	status_t err;
	wait_for_thread(folgeThread, &err);
	delete_sem(play_sem);
}
