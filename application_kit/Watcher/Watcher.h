/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

/* Watcher */

#ifndef WATCHER_H
#define WATCHER_H


#include <Application.h>
#include <VolumeRoster.h>

class WatcherApp : public BApplication {
	public:
							WatcherApp();
		virtual				~WatcherApp();
		void				ReadyToRun();
};


#include <Window.h>
#include "WatchItem.h"

class BOutlineListView;

class WatcherWindow : public BWindow {
	public:
							WatcherWindow();
		virtual				~WatcherWindow();
		bool				QuitRequested();
		
		void				MessageReceived(BMessage *msg);
		
	private:
	
		void				AddApps();
		void				AddVols();
		void				StartWatching();
		void				StopWatching();

		static	BListItem *	find_app(BListItem *item, void * team);
		static	BListItem *	find_vol_by_id(BListItem *item, void * id);
		static	BListItem *	find_vol_by_dev(BListItem *item, void * devi);
	
		AppItem *			fActiveApp;
	
		BStringItem *		fApps;
		BStringItem *		fVols;
		
		BOutlineListView *	fList;
		BVolumeRoster *		fVolRoster;
};

#endif

