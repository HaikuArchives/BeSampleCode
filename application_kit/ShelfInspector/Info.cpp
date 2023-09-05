/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Debug.h>

#include "main.h"
#include <Button.h>
#include <Dragger.h>
#include <Box.h>
#include <Alert.h>
#include <ScrollView.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <MenuField.h>
#include <ListItem.h>
#include <StringView.h>
#include <image.h>

/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
/*------------------------------------------------------------*/

// A couple helpers classes and functions.

struct IDItem : public BStringItem {
				IDItem(const char *name, int32 i);
	int32	id;
};

/*------------------------------------------------------------*/
struct match_info {
				match_info(image_id i) { id = i; found = false; };
	image_id	id;
	bool		found;
};

bool match_id(BListItem *item, void *data)
{
	match_info *mi = (match_info *) (data);
	IDItem *my = dynamic_cast<IDItem*>(item);
	if (my->id == (mi)->id) {
		(mi)->found = true;
		return true;
	}
	return false;
}

IDItem::IDItem(const char *name, int32 i)
	: BStringItem(name) 
{ id = i; };

/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
/*------------------------------------------------------------*/

TInfoWindow::TInfoWindow(BRect frame, const char *title)
	: BWindow(frame, title, B_TITLED_WINDOW, B_NOT_RESIZABLE),
		fTickToken(BMessenger(this), new BMessage(CMD_TICK), 1000000),
		fImportLoc(10,15)
{
	BRect	b = Bounds();
	BView *parent = new BView(b, "parent", B_FOLLOW_ALL, B_WILL_DRAW);
	parent->SetViewColor(216, 216, 216, 0);
	AddChild(parent);

	BRect	r = b;
	BRect	tmp_rect;
	
	r.InsetBy(5,10);
	r.bottom = r.top + 190;
	
	tmp_rect.Set(0,0,180,20);
	BPopUpMenu	*pop = new BPopUpMenu("Container Demo");
	pop->AddItem(new BMenuItem("Container Demo", new BMessage(CMD_SET_CONTAINER_TARGET)));
	pop->AddItem(new BMenuItem("Deskbar", new BMessage(CMD_SET_DESKBAR_TARGET)));
	pop->AddItem(new BMenuItem("Desktop Window", new BMessage(CMD_SET_DESKTOP_TARGET)));
	BMenuField	*mf = new BMenuField(tmp_rect, "popup", "Target Shelf:", pop);
	mf->SetDivider(67);
	
	BBox		*box = new BBox(r);
	BScrollView	*sv;
	BStringView	*stringv;
	
	box->SetLabel(mf);
	r = box->Bounds();
	r.InsetBy(10,12);
	r.top += 15;
	r.right -= 13;
	r.bottom = r.top + 15;
	stringv = new BStringView(r, "str1", "Loaded Replicants");
	box->AddChild(stringv);
	r.top = r.bottom + 3;
	r.bottom = r.top + 70;
	fReplicantList = new BListView(r, "rep_list", B_SINGLE_SELECTION_LIST, 
		B_FOLLOW_ALL);
	fReplicantList->SetSelectionMessage(new BMessage(CMD_REP_SELECTION_CHANGED));
	
	sv = new BScrollView("rep_scroll_view", fReplicantList, B_FOLLOW_ALL,
		B_WILL_DRAW, false, true);
	
	box->AddChild(sv);
	
	r.top = r.bottom + 10;
	
	BButton	*but;
	tmp_rect = r;
	
	but = new BButton(tmp_rect, "del", "Delete",
		new BMessage(CMD_DELETE_REPLICANT));
	but->ResizeToPreferred();
	but->SetEnabled(false);
	box->AddChild(but);
	fDeleteRep = but;
	
	tmp_rect = but->Frame();
	tmp_rect.left = tmp_rect.right + 10;
	but = new BButton(tmp_rect, "dup", "Copy to Container...",
		new BMessage(CMD_IMPORT_REQUEST));
	but->ResizeToPreferred();
	but->SetEnabled(false);
	box->AddChild(but);
	fCopyRep = but;
	
	r.top = tmp_rect.bottom + 15;

	r.bottom = r.top + 15;
	stringv = new BStringView(r, "str1", "Loaded Libraries");
	box->AddChild(stringv);
	r.top = r.bottom + 3;
	r.bottom = r.top + 70;
	fLibraryList = new BListView(r, "lib_list", B_SINGLE_SELECTION_LIST,
		B_FOLLOW_ALL);
	sv = new BScrollView("lib_scroll_view", fLibraryList, B_FOLLOW_ALL,
		B_WILL_DRAW, false, true);
	fLibraryList->SetSelectionMessage(new BMessage(CMD_LIB_SELECTION_CHANGED));
	
	box->AddChild(sv);
	
	r.top = r.bottom + 10;
	tmp_rect = r;
	
	but = new BButton(tmp_rect, "unload", "Unload...",
		new BMessage(CMD_UNLOAD_REQUEST));
	but->ResizeToPreferred();
	but->SetEnabled(false);
	box->AddChild(but);
	fUnloadLib = but;
	tmp_rect = but->Frame();

	box->ResizeTo(box->Bounds().Width(), tmp_rect.bottom + 10);
	
	parent->AddChild(box);
	
	tmp_rect = box->Frame();
	tmp_rect.top = tmp_rect.bottom + 5;
	tmp_rect.bottom = tmp_rect.top +25;
	tmp_rect.left += 30;
	stringv = new BStringView(tmp_rect, "str1",
		"<More goes here in my next newsletter>");
	parent->AddChild(stringv);
	
	PostMessage(CMD_SET_CONTAINER_TARGET);		// to initialize the lsit views
}

/*------------------------------------------------------------*/
TInfoWindow::~TInfoWindow()
{
}

/*------------------------------------------------------------*/

void TInfoWindow::Quit()
{
	BWindow::Quit();
}

/*------------------------------------------------------------*/

bool TInfoWindow::QuitRequested()
{
	long c = be_app->CountWindows();

	if (c == 1) {
		be_app->PostMessage(B_QUIT_REQUESTED);
	}
	return TRUE;
}

/*------------------------------------------------------------*/

void TInfoWindow::MessageReceived(BMessage *msg)
{		
	switch (msg->what) {
		case CMD_TICK: {
			UpdateLists(false);
			break;
		}
		case CMD_UNLOAD_REQUEST: {
			BAlert	*alert = new BAlert("",
				"Warning, unloading a library that is still in use is pretty bad. "
				"Are you sure you want to unload this library?",
				"Cancel", "Unload", NULL,
				B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
			BInvoker *inv = new BInvoker(new BMessage(CMD_UNLOAD_LIBRARY),
				this);
				
			alert->Go(inv);				
			break;
		}
		case CMD_UNLOAD_LIBRARY: {
			// This message was posted by the Alert above. If 'which' is 1 then
			// the user pressed the Import button. Otherwise they pressed Cancel.
			int32 r = msg->FindInt32("which");
			if (r == 1) {
				int32	sel = fLibraryList->CurrentSelection();
				IDItem	*item = dynamic_cast<IDItem*>(fLibraryList->ItemAt(sel));
				ASSERT(sel >= 0);
				ASSERT(item);
				unload_add_on(item->id);
			}
			break;
		}
		case CMD_DELETE_REPLICANT: {
			int32	sel = fReplicantList->CurrentSelection();
			IDItem	*item = dynamic_cast<IDItem*>(fReplicantList->ItemAt(sel));
			ASSERT(sel >= 0);
			ASSERT(item);
			DeleteReplicant(item->id);
			break;
		}
		case CMD_IMPORT_REQUEST: {
			BAlert	*alert = new BAlert("",
				"Warning, not all replicants are importable. Importing a "
				"replicant can crash the Container Demo application. Are you "
				"willing to give it a try?", "Cancel", "Import", NULL,
				B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
			BInvoker *inv = new BInvoker(new BMessage(CMD_IMPORT_REPLICANT),
				this);
				
			alert->Go(inv);				
			break;
		}
		case CMD_IMPORT_REPLICANT: {
			// This message was posted by the Alert above. If 'which' is 1 then
			// the user pressed the Import button. Otherwise they pressed Cancel.
			int32 r = msg->FindInt32("which");
			if (r == 1) {
				int32	sel = fReplicantList->CurrentSelection();
				IDItem	*item = dynamic_cast<IDItem*>(fReplicantList->ItemAt(sel));
				ASSERT(sel >= 0);
				ASSERT(item);
				ImportReplicant(item->id);
			}
			break;
		}
		case CMD_SET_DESKBAR_TARGET:
		case CMD_SET_DESKTOP_TARGET:
		case CMD_SET_CONTAINER_TARGET: {
			fTarget = MessengerForTarget(msg->what);
			UpdateLists(true);
			break;
		}
		case CMD_REP_SELECTION_CHANGED: {
			bool	enabled;
			enabled = (fReplicantList->CurrentSelection() >= 0);
			fDeleteRep->SetEnabled(enabled);
			fCopyRep->SetEnabled(enabled);
			break;
		}
		case CMD_LIB_SELECTION_CHANGED: {
			bool	enabled;
			enabled = (fLibraryList->CurrentSelection() >= 0);
			fUnloadLib->SetEnabled(enabled);
			break;
		}
		case B_ABOUT_REQUESTED: {
			BAlert	*a = new BAlert("", "About Replicant window!", "OK");
			a->Go(NULL);
			break;
		}
		default:
			BWindow::MessageReceived(msg);
	}
}

/*------------------------------------------------------------*/

void TInfoWindow::EmptyLists()
{
	fReplicantList->MakeEmpty();
	fLibraryList->MakeEmpty();
}

/*------------------------------------------------------------*/

void TInfoWindow::UpdateLists(bool make_empty)
{
	bool	deleted_something = false;
	
	if (!fTarget.IsValid()) {
		EmptyLists();
		PostMessage(CMD_REP_SELECTION_CHANGED);
		PostMessage(CMD_LIB_SELECTION_CHANGED);
		return;
	}
	if (make_empty) {
		EmptyLists();
		deleted_something = true;
	}
	
	/*
	 I'm not worried about the allgorithms used below to maintain the 2 lists.
	 That isn't the point of this sample app.
	 */

	image_info	info;

	if (!make_empty) {
		// walk through the current list of images and remove any that are
		// no longer loaded
		IDItem	*item;
		int32	i = fLibraryList->CountItems();
		while ((item = dynamic_cast<IDItem*>(fLibraryList->ItemAt(--i))) != NULL) {
			image_id	id = (image_id) item->id;
			if (get_image_info(id, &info) != B_OK) {
				fLibraryList->RemoveItem(item);
				delete item;
				deleted_something = true;
			}
		}
	}
	
	// get all the images for the 'team' of the target. If the image isn't in the
	// list then add
	team_id		team = fTarget.Team();
	int32		cookie = 0;
	
	while (get_next_image_info(team, &cookie, &info) == B_OK) {
		match_info	mi(info.id);
		fLibraryList->DoForEach(match_id, &mi);
		if (!mi.found) {
			fLibraryList->AddItem(new IDItem(info.name, info.id));
		}
	}
	
	// Now it's time to deal with the replicant list
	
	if (!make_empty) {
		// walk through the current list of replicants and remove any that are
		// no longer loaded
		IDItem	*item;
		int32	i = fReplicantList->CountItems();
		while ((item = dynamic_cast<IDItem*>(fReplicantList->ItemAt(--i))) != NULL) {
			int32	uid = item->id;
			if (IsReplicantLoaded(uid) == false) {
				fReplicantList->RemoveItem(item);
				delete item;
				deleted_something = true;
			}
		}
	}
	
	// Now get all the replicants from the shelf and make sure that they are in
	// the list
	int32	index = 0;
	int32	uid;
	while ((uid = GetReplicantAt(index++)) >= B_OK) {
		// if this uid is already in the list then skip it
		match_info	mi(uid);
		fReplicantList->DoForEach(match_id, &mi);
		if (mi.found) {
			continue;
		}
		
		BMessage	rep_info;
		if (GetReplicantName(uid, &rep_info) != B_OK) {
			continue;
		}
		const char *name;
		if (rep_info.FindString("result", &name) == B_OK) {
			fReplicantList->AddItem(new IDItem(name, uid));
		}
	}
	
	if (deleted_something) {
		PostMessage(CMD_REP_SELECTION_CHANGED);
		PostMessage(CMD_LIB_SELECTION_CHANGED);
	}
}

/*------------------------------------------------------------*/

status_t TInfoWindow::GetReplicantName(int32 uid, BMessage *reply) const
{
	/*
	 We send a message to the target shelf, asking it for the Name of the 
	 replicant with the given unique id.
	 */
	 
	BMessage	request(B_GET_PROPERTY);
	BMessage	uid_specifier(B_ID_SPECIFIER);	// specifying via ID
	status_t	err;
	status_t	e;
	
	request.AddSpecifier("Name");		// ask for the Name of the replicant
	
	// IDs are specified using code like the following 3 lines:
	uid_specifier.AddInt32("id", uid);
	uid_specifier.AddString("property", "Replicant");
	request.AddSpecifier(&uid_specifier);
	
	if ((err = fTarget.SendMessage(&request, reply)) != B_OK)
		return err;
	
	if (((err = reply->FindInt32("error", &e)) != B_OK) || (e != B_OK))
		return err ? err : e;
	
	return B_OK;
}

/*------------------------------------------------------------*/

bool TInfoWindow::IsReplicantLoaded(int32 uid) const
{
	/*
	 determine if the specified replicant (the unique ID of the replicant)
	 still exists in the target container/shelf. If we can get the name then the 
	 replicant still exists.
	 */
	BMessage	reply;	
	status_t	err = GetReplicantName(uid, &reply);
	return (err == B_OK);
}

/*------------------------------------------------------------*/

int32 TInfoWindow::GetReplicantAt(int32 index) const
{
	/*
	 So here we want to get the Unique ID of the replicant at the given index
	 in the target Shelf.
	 */
	 
	BMessage	request(B_GET_PROPERTY);		// We're getting the ID property
	BMessage	reply;
	status_t	err;
	
	request.AddSpecifier("ID");					// want the ID
	request.AddSpecifier("Replicant", index);	// of the index'th replicant
	
	if ((err = fTarget.SendMessage(&request, &reply)) != B_OK)
		return err;
	
	int32	uid;
	if ((err = reply.FindInt32("result", &uid)) != B_OK) 
		return err;
	
	return uid;
}

/*------------------------------------------------------------*/

BMessenger TInfoWindow::MessengerForTarget(type_code w) const
{
	/*
	 This function determines the BMessenger to the various Shelf objects
	 that this app can talk with.
	 */
	BMessage	request(B_GET_PROPERTY);
	BMessenger	to;
	BMessenger	result;

	request.AddSpecifier("Messenger");
	request.AddSpecifier("Shelf");
	
	switch (w) {
		case CMD_SET_DESKBAR_TARGET: {
			// In the Deskbar the Shelf is in the View "Status" in Window "Deskbar"
			request.AddSpecifier("View", "Status");
			request.AddSpecifier("Window", "Deskbar");
			to = BMessenger("application/x-vnd.Be-TSKB", -1);
			break;
		}
		case CMD_SET_DESKTOP_TARGET: {
			// The Desktop is owned by Tracker and the Shelf is in the
			// View "PoseView" in Window "Desktop"
			request.AddSpecifier("View", "PoseView");
			request.AddSpecifier("Window", "Desktop");
			to = BMessenger("application/x-vnd.Be-TRAK", -1);
			break;
		}
		case CMD_SET_CONTAINER_TARGET: {
			// In the COntainer Demo app the View "MainView" in the only window
			// is the Shelf.
			request.AddSpecifier("View", "MainView");
			request.AddSpecifier("Window", (int32) 0);
			to = BMessenger("application/x-vnd.Be-MYTE", -1);
			// This demo app isn't worried about the Container app going away
			// (quitting) unexpectedly.
			break;
		}
	}
	
	BMessage	reply;
	
	if (to.SendMessage(&request, &reply) == B_OK) {
		reply.FindMessenger("result", &result);
	}
	return result;
}

/*------------------------------------------------------------*/

status_t TInfoWindow::DeleteReplicant(int32 uid)
{
	// delete the given replicant from the current target shelf
	 
	BMessage	request(B_DELETE_PROPERTY);		// Delete
	BMessage	uid_specifier(B_ID_SPECIFIER);	// specifying via ID
	BMessage	reply;
	status_t	err;
	status_t	e;
	
	// IDs are specified using code like the following 3 lines:
	uid_specifier.AddInt32("id", uid);
	uid_specifier.AddString("property", "Replicant");
	request.AddSpecifier(&uid_specifier);
	
	if ((err = fTarget.SendMessage(&request, &reply)) != B_OK)
		return err;
	
	if ((err = reply.FindInt32("error", &e)) != B_OK)
		return err;
	
	return e;
}

/*------------------------------------------------------------*/

status_t TInfoWindow::ImportReplicant(int32 uid)
{
	// Import the given replicant from the current target shelf
	// That is get a copy and recreate it in the "Container" window of
	// this app.
	 
	BMessage	request(B_GET_PROPERTY);		// Get will return the archive msg
	BMessage	uid_specifier(B_ID_SPECIFIER);	// specifying via ID
	BMessage	reply;
	status_t	err;
	status_t	e;
			
	// IDs are specified using code like the following 3 lines:
	uid_specifier.AddInt32("id", uid);
	uid_specifier.AddString("property", "Replicant");
	request.AddSpecifier(&uid_specifier);
	
	if ((err = fTarget.SendMessage(&request, &reply)) != B_OK)
		return err;
	
	if (((err = reply.FindInt32("error", &e)) != B_OK) || (e != B_OK))
		return err;
	
	// OK, let's get the archive message
	BMessage	data;
	reply.FindMessage("result", &data);
	
	// Now send this to the container window. If someone closed it then the
	// Send will fail. Oh well.
	BMessenger	mess = MessengerForTarget(CONTAINER_MESSENGER);
	BMessage	msg(B_CREATE_PROPERTY);
	
	msg.AddMessage("data", &data);
	
	// As this is a Demo I'm not going to worry about some fancy layout
	// algorithm. Just keep placing new replicants going down the window
	msg.AddPoint("location", fImportLoc);
	fImportLoc.y += 40;
	
	return mess.SendMessage(&msg, &reply);
}

/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
/*------------------------------------------------------------*/

