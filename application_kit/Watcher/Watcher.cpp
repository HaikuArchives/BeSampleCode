/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

/* Watcher.cpp */

#include "Watcher.h"
#include "WatchItem.h"

#include <Roster.h>
#include <VolumeRoster.h>
#include <NodeMonitor.h>
#include <Volume.h>
#include <Directory.h>
#include <Mime.h>
#include <OutlineListView.h>
#include <List.h>
#include <OS.h>
#include <Messenger.h>

#include <stdio.h>

WatcherApp::WatcherApp()
	: BApplication("application/x-vnd.BeDTS-Watcher")
{
}


WatcherApp::~WatcherApp()
{
}

void 
WatcherApp::ReadyToRun()
{
	//all the work is done in the window so start it up
	WatcherWindow *watcher = new WatcherWindow();
	watcher->Show();
}

WatcherWindow::WatcherWindow()
		: BWindow(BRect(50,50,350,500), "Watcher", B_TITLED_WINDOW, 0),
			fActiveApp(NULL), fApps(new BStringItem("Application (Team ID)")), 
			fVols(new BStringItem("( volume_id | device id ) Volume Name")),
			fList(new BOutlineListView(Bounds(), "WatcherList")), fVolRoster(NULL) 
{

	fList->AddItem(fApps);
	fList->AddItem(fVols);

	//add the initial app and volume list	
	AddApps();
	AddVols();
	//request update messages
	StartWatching();
	//add the display	
	AddChild(fList);
}


WatcherWindow::~WatcherWindow()
{
	//stop the update messages
	StopWatching();
	//get rid of the volume roster
	if (fVolRoster) {
		delete fVolRoster; fVolRoster = NULL;
	}
}

bool 
WatcherWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

void 
WatcherWindow::AddApps()
{
	//get information on the current active app
	app_info active;
	be_roster->GetActiveAppInfo(&active);

	//get the list of currently running applications
	BList app_list;
	be_roster->GetAppList(&app_list);
	int32 count = app_list.CountItems();
	
	//for every application, create a new AppItem
	//activate it if it is the active app
	//add it to the display
	
	for (int32 ix = 0; ix < count; ix++) {
		team_id team = (team_id) app_list.ItemAt(ix);
		AppItem *item = new AppItem(team);
		//if the active team, activate the AppItem
		if (active.team == team) {
			item->Activate();
			fActiveApp = item;
		}
		fList->AddUnder(item, fApps);
	}
}

void 
WatcherWindow::AddVols()
{
	fVolRoster = new BVolumeRoster();
	fVolRoster->Rewind();
	BVolume vol;

	//step through the currently mounted volumes
	//create VolItems for the persistant volumes
	//add them to the display
	while (fVolRoster->GetNextVolume(&vol) != B_BAD_VALUE) {
		if (vol.InitCheck() == B_OK &&
		    vol.IsPersistent()) 
		{
			VolItem *item = new VolItem(&vol);
			fList->AddUnder(item, fVols);
		}
	}
}

void 
WatcherWindow::StartWatching()
{
	BMessenger msgr(this);
	//tell the BRoster to send us messages about launching, quitting and activation of apps
	be_roster->StartWatching(msgr, B_REQUEST_LAUNCHED | B_REQUEST_QUIT | B_REQUEST_ACTIVATED);
	//the the volume roster to inform us of mounting and unmounting of volumes
	fVolRoster->StartWatching(msgr);
}

void 
WatcherWindow::StopWatching()
{
	BMessenger msgr(this);
	//tell the BRoster to stop sending us app updates
	be_roster->StopWatching(msgr);
	//tell the volume roster to stop sending us volume updates
	fVolRoster->StopWatching();
}



void 
WatcherWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what) {

		//BVolumeRoster messages
		case B_NODE_MONITOR:
		{
			int32 opcode;
			if (msg->FindInt32("opcode", &opcode) != B_OK)
							break;
			
			//volume mounted message
			if (opcode == B_DEVICE_MOUNTED) {
				//find the device of the mounted volume
				dev_t device = 0;
				if (msg->FindInt32("new device", &device) != B_OK)
							break;				

				//get the volume_id from the newly mounted volume
				//function is in WatchItem.cpp
				uint64 id = 0;
				id = get_volume_id(device);
				
				//find the VolItem (if any) that matches this volume_id
				VolItem *volume = NULL;
				volume = (VolItem *) fList->EachItemUnder(fVols, true, find_vol_by_id, (void *) &id);
				
				
				if (volume) {
					//mark the VolItem as mounted
					volume->Mount(device);
				} else {
					//create a new VolItem and add it
					BVolume vol(device);
					volume = new VolItem(&vol);
					fList->AddUnder(volume, fVols);
				};
				//invalidate the item so it displays correctly
				fList->InvalidateItem(fList->FullListIndexOf(volume));
			} 
			//volume unmounted message
			else if (opcode == B_DEVICE_UNMOUNTED) {
				//find the device of the mounted volume
				dev_t device = 0;
				if (msg->FindInt32("device", &device) != B_OK)
							break;				
				//find the appropriate VolItem by device number
				//we cannot check by id as the volume is already unmounted
				//and we cannot get at the attribute
				VolItem *volume = NULL;
				volume = (VolItem *) fList->EachItemUnder(fVols, true, find_vol_by_dev, (void *) device);
				if (volume) {
					//mark the volume as Unmounted and Invalidate the display
					volume->Unmount();
					fList->InvalidateItem(fList->FullListIndexOf(volume));
				}				
			}
			break;
		}
		
		
		//BRoster messages
		case B_SOME_APP_LAUNCHED:
		{
			//create a new AppItem for the newly launched team
			team_id team;
			msg->FindInt32("be:team", &team);
			AppItem *item = new AppItem(team);
			fList->AddUnder(item, fApps);
			break;
		}
		case B_SOME_APP_QUIT:
		{
			team_id team;
			msg->FindInt32("be:team", &team);
			
			//find the AppItem for the quitting app
			AppItem *app = (AppItem *) fList->EachItemUnder(fApps, true, find_app, (void *) team);
			if (app) {
				//if the active app, mark as deactivated and clear the active app pointer
				if (app == fActiveApp) {
					app->Deactivate();
					fActiveApp = NULL;
				}
				
				//remove and delete the AppItem
				fList->RemoveItem(app);
				delete app;
			}
			break;
		}
		case B_SOME_APP_ACTIVATED:
		{
			team_id team;
			msg->FindInt32("be:team", &team);
			//find the AppItem for the activated application
			AppItem *app = (AppItem *) fList->EachItemUnder(fApps, true, find_app, (void *) team);
			if (app) {
				//mark the old active AppItem as deactivated
				if (fActiveApp) {
					fActiveApp->Deactivate();
					fList->InvalidateItem(fList->FullListIndexOf(fActiveApp));
				}
				//activate the new AppItem, update the active app pointer
				app->Activate();
				fActiveApp = app;
				fList->InvalidateItem(fList->FullListIndexOf(app));
			}
			break;
		}
		default:
			BWindow::MessageReceived(msg);	
	}
	
}

int main() {
	WatcherApp app;
	app.Run();
	return 0;
}

BListItem * 
WatcherWindow::find_app(BListItem *item, void * arg)
{
	AppItem *app = dynamic_cast<AppItem*>(item);
	team_id team = (team_id) arg;
	
	if (app) {
		if (app->Team() == team) {
			return app;
		} else return NULL;
	} else return NULL;
}


BListItem *
WatcherWindow::find_vol_by_id(BListItem *item, void * id)
{
	VolItem *vol = dynamic_cast<VolItem *>(item);
	uint64 vol_id = *(uint64 *)id;
	if (vol) {
		if (vol->ID() == vol_id) {
			return vol;
		} else return NULL;	
	} else return NULL;
}

BListItem *
WatcherWindow::find_vol_by_dev(BListItem *item, void * dev)
{
	VolItem *vol = dynamic_cast<VolItem *>(item);
	dev_t device = (dev_t) dev;
	if (vol) {
		if (vol->Device() == device) {
			return vol;
		} else return NULL;	
	} else return NULL;
}
