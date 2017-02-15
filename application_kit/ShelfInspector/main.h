/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Application.h>
#include <Messenger.h>
#include <Window.h>
#include <Shelf.h>
#include <ListView.h>
#include <MessageRunner.h>
#include <Button.h>

/*------------------------------------------------------------*/

extern const char *INFO_WINDOW_TITLE;

enum {
	CMD_SET_DESKBAR_TARGET		= 'sdes',
	CMD_SET_DESKTOP_TARGET		= 'strk',
	CMD_SET_CONTAINER_TARGET	= 'smee',
	CMD_DELETE_REPLICANT		= 'delr',
	CMD_IMPORT_REPLICANT 		= 'dupr',
	CMD_IMPORT_REQUEST	 		= 'ireq',
	CMD_REP_SELECTION_CHANGED	= 'rslc',
	CMD_LIB_SELECTION_CHANGED	= 'lslc',
	CMD_UNLOAD_REQUEST			= 'ureq',
	CMD_UNLOAD_LIBRARY			= 'unld',
	
	DESKBAR_MESSENGER			= CMD_SET_DESKBAR_TARGET,
	DESKTOP_MESSENGER			= CMD_SET_DESKTOP_TARGET,
	CONTAINER_MESSENGER			= CMD_SET_CONTAINER_TARGET,

	CMD_TICK					= 'tick',	
};
	
/*------------------------------------------------------------*/

class TInfoWindow : public BWindow {
public:
					TInfoWindow(BRect frame, const char *);
					~TInfoWindow();
virtual void		MessageReceived(BMessage *msg);
virtual bool		QuitRequested();
virtual void		Quit();

private:
		// These are the interesting functions showing how you can 
		// interact via BMessages with a Shelf/Container view
		BMessenger	MessengerForTarget(type_code w) const;
		int32		GetReplicantAt(int32 index) const;
		status_t	GetReplicantName(int32 uid, BMessage *result) const;
		status_t	DeleteReplicant(int32 uid);
		status_t	ImportReplicant(int32 uid);
		
		bool		IsReplicantLoaded(int32 uid) const;
		
		void		UpdateLists(bool make_empty);
		void		EmptyLists();
		
		BListView		*fReplicantList;
		BListView		*fLibraryList;
		BMessenger		fTarget;
		BMessageRunner	fTickToken;
		BButton			*fDeleteRep;
		BButton			*fCopyRep;
		BButton			*fUnloadLib;
		BPoint			fImportLoc;
};
