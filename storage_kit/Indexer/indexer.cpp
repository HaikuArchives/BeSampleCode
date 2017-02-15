/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#include "indexer.h"

#include <Alert.h>
#include <Directory.h>
#include <VolumeRoster.h>
#include <Volume.h>
#include <posix/stdio.h>
#include <Path.h>

#include "Defs.h"
#include "vInfoView.h"
#include "dInfoView.h"
#include "fInfoView.h"
#include "lInfoView.h"


Indexer::Indexer()
			: BApplication("application/x-vnd.BeDTS-Indexer")
{
}


Indexer::~Indexer()
{
}

void 
Indexer::ReadyToRun()
{
}

void 
Indexer::MessageReceived(BMessage *msg)
{
}

void 
Indexer::ArgvReceived(int32 argc, char **argv)
{
	for (int i = 1; i < argc; i++) {
		entry_ref ref;
		if (get_ref_for_path(argv[i], &ref) == B_OK) EvaluateRef(ref);
	}
}

void 
Indexer::RefsReceived(BMessage *msg)
{
	uint32 		type;
	int32 		count;
	entry_ref 	ref;
	msg->GetInfo("refs", &type, &count);
	if (type != B_REF_TYPE) return;
	
	for (int32 i = --count; i >= 0; i--) {
		if (msg->FindRef("refs", i, &ref) == B_OK) {
			EvaluateRef(ref);
		}
	}
}

void 
Indexer::AboutRequested()
{
	BAlert *alert = new BAlert("About Indexer", ABOUT_INFO, "Dig It!");
	alert->Go(NULL);
}

status_t 
Indexer::EvaluateRef(entry_ref &ref)
{
	struct stat st;
	BEntry entry;
	
	if (entry.SetTo(&ref, false) != B_OK) return B_ERROR;		
	
	if (entry.GetStat(&st) != B_OK) return B_ERROR;
	
	
	if (S_ISLNK(st.st_mode)) {
		return HandleLink(ref, st);
	}
	else if (S_ISREG(st.st_mode)) return HandleFile(ref, st);
	
	else if (S_ISDIR(st.st_mode)) {
		BDirectory dir;
		if (dir.SetTo(&ref) != B_OK) return B_ERROR;
		
		if (dir.IsRootDirectory()) return HandleVolume(ref, st, dir);
		
		else return HandleDirectory(ref, st, dir);
	}
	else return B_ERROR;
}

status_t 
Indexer::HandleVolume(entry_ref &ref, struct stat &st, BDirectory &dir)
{
	BVolumeRoster vol_roster;
	BVolume vol;
	BDirectory root_dir;
	
	while (vol_roster.GetNextVolume(&vol) == B_NO_ERROR) {
		vol.GetRootDirectory(&root_dir);
		if (root_dir == dir) break;
	}
	
	if (dir != root_dir) {
		BAlert *alert = new BAlert("", "Device for RootDirectory not found", "Cancel");
		alert->Go(NULL);
		return B_ERROR;
	}

	vInfoView *vinfo= new vInfoView(vol, st);
	if (vinfo->InitCheck() == B_OK) {
		char *name = vinfo->GetName();
		char title[64];
		sprintf(title,"%s info", name);

		BRect rect = vinfo->Bounds();
		rect.OffsetTo(70,50);
		
		BWindow *win = new BWindow(rect, title, B_TITLED_WINDOW, B_NOT_RESIZABLE);
		
		win->AddChild(vinfo);
		win->Show();
		return B_NO_ERROR;
	}
	else {
		delete vinfo;
		return B_ERROR;
	}
}

status_t 
Indexer::HandleDirectory(entry_ref &ref, struct stat &st, BDirectory &dir)
{

	dInfoView *dinfo= new dInfoView(ref, st, dir);
	if (dinfo->InitCheck() == B_OK) {
		char *name = dinfo->GetName();
		char title[64];
		sprintf(title,"%s info", name);

		BRect rect = dinfo->Bounds();
		rect.OffsetTo(70,50);
		
		BWindow *win = new BWindow(rect, title, B_TITLED_WINDOW, B_NOT_RESIZABLE);
		win->AddChild(dinfo);
		win->Show();
		return B_NO_ERROR;
	}
	else {
		delete dinfo;
		return B_ERROR;
	}
}

status_t 
Indexer::HandleFile(entry_ref &ref, struct stat &st)
{

	fInfoView *finfo= new fInfoView(ref, st);
	if (finfo->InitCheck() == B_OK) {
		char *name = finfo->GetName();
		char title[64];
		sprintf(title,"%s info", name);

		BRect rect = finfo->Bounds();
		rect.OffsetTo(70,50);
		
		BWindow *win = new BWindow(rect, title, B_TITLED_WINDOW, B_NOT_RESIZABLE);

		win->AddChild(finfo);
		win->Show();
		return B_NO_ERROR;
	}
	else {
		delete finfo;
		return B_ERROR;
	}
}

status_t 
Indexer::HandleLink(entry_ref &ref, struct stat &st)
{
	lInfoView *linfo= new lInfoView(ref, st);
	if (linfo->InitCheck() == B_OK) {
		char *name = linfo->GetName();
		char title[64];
		sprintf(title,"%s info", name);

		BRect rect = linfo->Bounds();
		rect.OffsetTo(70,50);
	
		BWindow *win = new BWindow(rect, title, B_TITLED_WINDOW, B_NOT_RESIZABLE);
		win->AddChild(linfo);
		win->Show();
		return B_NO_ERROR;
	}
	else {
		delete linfo;
		return B_ERROR;
	}
}

int main(int, char**) {
	Indexer indx;
	indx.Run();
	return 0;	
}
