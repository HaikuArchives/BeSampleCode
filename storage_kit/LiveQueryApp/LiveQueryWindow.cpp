/* LiveQueryWindow.cpp */
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "LiveQueryWindow.h"

#define DEBUG 1

#include <Debug.h>
#include <Query.h>
#include <ListView.h>
#include <StringView.h>
#include <TextControl.h>
#include <Box.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <Button.h>
#include <ScrollView.h>
#include <TextView.h>
#include <Application.h>
#include <Volume.h>
#include <Path.h>
#include <NodeMonitor.h>
#include <Roster.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void
live_ref_to_stream(live_ref *rec)
{
	printf("dev:%ld | node:%Ld | pdev:%ld | pnode:%Ld |  name: %s | item: %p | status:%ld | state: %ld\n",
		rec->dev, rec->node, rec->pdev, rec->pnode, rec->name, rec->item, rec->status, rec->state);
}

int main(int, char**) {
	BApplication app("application/x-vnd.BeDTS-LiveQueryApp");
	LiveQueryWindow *window = new LiveQueryWindow();
	window->Show();
	app.Run();
	return 0;
}

LiveQueryWindow::LiveQueryWindow()
	: BWindow(BRect(50,50,394,449), "LiveQueryApp", B_TITLED_WINDOW, B_NOT_RESIZABLE),
	fQueryText(NULL), fValidView(NULL), fNameString(NULL), fCountString(NULL),
	fQuery(new BQuery()), fValidList(50), fZombieList(), fTrackingLock("TrackingLock"),
	fThreadID(-1), fFileType(FILES_TYPE), fCount(0), fOKToQuit(true), fTryingToQuit(false)
{
	/* create interface */
	//query definitions
	BBox *top = new BBox(Bounds(), "top", B_FOLLOW_ALL, B_WILL_DRAW|B_NAVIGABLE, B_PLAIN_BORDER);
	AddChild(top); 
	
	BPopUpMenu * pop_up = new BPopUpMenu("FileType");
	BMenuItem *item = new BMenuItem("Files", new BMessage(FILES_TYPE));
	item->SetMarked(true);
	pop_up->AddItem(item);
	item = new BMenuItem("Folders", new BMessage(FOLDERS_TYPE));
	pop_up->AddItem(item);
	item = new BMenuItem("Source", new BMessage(SOURCE_TYPE));
	pop_up->AddItem(item);
	BMenuField * menu_field = new BMenuField(BRect(25, 5, 175, 17), "FileType", "File Type:", pop_up);
	menu_field->SetDivider(be_bold_font->StringWidth("File Type:"));
	top->AddChild(menu_field);

	BStringView *method = new BStringView(BRect(250, 12, 314, 23), "Method", "Method: Live");
	top->AddChild(method);

	fNameString = new BTextControl(BRect(25,33, 314, 52), "Name String", "Name Contains:", "query", NULL);
	fNameString->SetDivider(be_bold_font->StringWidth("Name Contains:"));
	top->AddChild(fNameString);

	BButton *button = new BButton(BRect(25,60, 314, 87), "Fetch", "Fetch Query Results", new BMessage(FETCH));
	button->MakeDefault(true);
	top->AddChild(button);

	//results
	BStringView *label = new BStringView(BRect(25,95,314, 107), "Results", "Query String:");
	label->SetAlignment(B_ALIGN_CENTER);
	top->AddChild(label);

	/* replace with a BTextView? */
	BRect rect(25, 109, 314, 142);
	BRect text_rect = rect;
	text_rect.OffsetTo(B_ORIGIN);
	text_rect.InsetBy(1,1);
	fQueryText = new BTextView(rect, "QueryText", text_rect, B_FOLLOW_ALL, B_WILL_DRAW | B_NAVIGABLE);
	fQueryText->SetViewColor(220,220,220);
	fQueryText->MakeEditable(false);
	fQueryText->SetAlignment(B_ALIGN_CENTER);
	BFont font;
	fQueryText->GetFont(&font);
	font.SetSize(font.Size() -2);
	fQueryText->SetFont(&font);
	top->AddChild(fQueryText);
	
	rect.Set(15,150,327,389);
	BBox *item_box = new BBox(rect, "Matching Paths");
	item_box->SetLabel("Matching Paths");
	rect.OffsetTo(5,25);
	rect.right = 293;
	rect.bottom = 221;

	fCountString = new BStringView(BRect(250, 10, 299, 22), "CountString", "Count: 0");
	fCountString->GetFont(&font);
	font.SetSize(font.Size() -2);
	fCountString->SetFont(&font);
	item_box->AddChild(fCountString);
	
	fValidView = new BListView(rect, "ValidView");
	fValidView->GetFont(&font);
	size_t size = (size_t)font.Size();
	if (size <=10) size -= 1;
	else size -= 2;
	font.SetSize(size);
	fValidView->SetFont(&font);

	BScrollView *scroller = new BScrollView("Scroller", fValidView, B_FOLLOW_LEFT|B_FOLLOW_TOP,0, true, true);
	BScrollBar *scroll = scroller->ScrollBar(B_HORIZONTAL);
	scroll->SetRange(0, 200);
	item_box->AddChild(scroller);
	top->AddChild(item_box);
}


LiveQueryWindow::~LiveQueryWindow()
{
	for (;;) {
		if (!fOKToQuit)
			fTryingToQuit = true;
		else {
			break;
		}
		snooze(10000);
	}
	ClearResults();
	delete fQuery;
}

void 
LiveQueryWindow::MessageReceived(BMessage *msg)
{
	int32 opcode;
	switch(msg->what) {

		case FILES_TYPE:
		case FOLDERS_TYPE:
		case SOURCE_TYPE:
		{
			fFileType = msg->what;
			break;
		}
		case FETCH:
		{
			ClearResults();
			fOKToQuit = false;
			fThreadID = spawn_thread(QueryThreadFunc, "query_thread", B_NORMAL_PRIORITY, this);
			resume_thread(fThreadID);
			break;
		}
		case B_QUERY_UPDATE:
//			printf("B_QUERY_UPDATE recvd\n");
			msg->FindInt32("opcode", &opcode);
			if(opcode == B_ENTRY_REMOVED) {
				//an entry no longer meets the query
				printf("B_QUERY_UPDATE:B_ENTRY_REMOVED called\n");
				EntryRemoved(msg);
			} else if (opcode == B_ENTRY_CREATED) {
				//an entry now meets the query
				printf("B_QUERY_UPDATE:B_ENTRY_CREATED called\n");
				EntryCreated(msg);
			} else printf("unknown opcode\n");
			
			break;
		
		case B_NODE_MONITOR:
			printf("B_NODE_MONITOR rcvd\n");
			msg->FindInt32("opcode", &opcode);
			if (opcode == B_ENTRY_REMOVED) EntryRemoved(msg);
			else if (opcode == B_ENTRY_MOVED) EntryMoved(msg);
			else EntryChanged(opcode, msg);
			
		default:
			BWindow::MessageReceived(msg);
			break;
	}
}

bool 
LiveQueryWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

int32 
LiveQueryWindow::QueryThreadFunc(void *arg)
{
	LiveQueryWindow * win = (LiveQueryWindow *)arg;
	win->FetchLive();
	return B_OK;
}

void 
LiveQueryWindow::FetchLive()
{
	// we have to lock our window first to be able to
	// access it safely
	if (!Lock())
		// the window is dead, bail
		return;
	
	
	//disable the button
	BButton *button = (BButton *) FindView("Fetch");
	button->SetEnabled(false);

	// save a local pointer to fQuery so that we can iterate it with the
	// window unlocked
	BQuery *query = fQuery;

	//make sure the query is cleared
	query->Clear();
	
	/* set up the query */
	//volume
	app_info info;
	be_app->GetAppInfo(&info);
	//use the ref of the application binary
	//use the device from this ref to create a volume
	BVolume vol(info.ref.device);
	query->SetVolume(&vol);
	
	//look for source code, folders, or all files (non-folders)
	char *type = NULL;
	query->PushAttr("BEOS:TYPE");
	if (fFileType == SOURCE_TYPE) type = "text/x-source-code";
	else type = "application/x-vnd.Be-directory";
	query->PushString(type);
	if (fFileType == FILES_TYPE) query->PushOp(B_NE);
	else query->PushOp(B_EQ);
	
	query->PushAttr("name");
	query->PushString(fNameString->Text());
	query->PushOp(B_CONTAINS);
	query->PushOp(B_AND);	
	
	//make the query live
	query->SetTarget(this);

	//display the new query string
	size_t len = query->PredicateLength();
	char *string = (char *) malloc(len + 1);
	query->GetPredicate(string, len + 1);
	fQueryText->SetText(string);

	free(string);
	
	/* get the query results */
	query->Fetch();

	// Unlock for a little bit while we call GetNextDirents
	Unlock();


	/* we need to cache a bunch of low-level info from the query */
	/* as such it is best that we use GetNextDirents to get the query */
	/* results as it gives us more info */
	dirent *dent;
	char buffer[1024];
	while (query->GetNextDirents((dirent *)buffer, 1024, 1) != 0) {
		dent = (dirent *) buffer;
		live_ref *rec = NULL;
		
		//search for this ref
		node_ref node;
		node.device = dent->d_dev;
		node.node = dent->d_ino;


		// lock to access shared state
		if (fTryingToQuit) {
			fOKToQuit = true;
			return;
		}
		if (!Lock())
			// shouldn't really happen
			return;
		
		if (FindCachedEntry(node, &rec, &fValidList) == B_OK) {
			//update the item
			printf("already in ValidList\n");
			
			if (rec->pnode != dent->d_pino) rec->pnode = dent->d_pino;
			rec->status = B_OK;
			if (strcmp(rec->name, dent->d_name) != 0) {
				free(rec->name);
				rec->name = strdup(dent->d_name);
			}
			rec->state = B_OK;

			UpdateEntry(rec, true);			
		} else if (FindCachedEntry(node, &rec, &fZombieList) == B_OK) {
			//update the item
			//move the item to the valid list if appropriate
			printf("already in ZombieList\n");

			if (rec->pnode != dent->d_pino) rec->pnode = dent->d_pino;
			rec->status = B_OK;
			if (strcmp(rec->name, dent->d_name) != 0) {
				free(rec->name);
				rec->name = strdup(dent->d_name);
			}
			rec->state = B_OK;

			UpdateEntry(rec, false);
		} else {	
			//create a new live ref
			rec = (live_ref *) malloc(sizeof(live_ref));
			rec->dev = dent->d_dev;
			rec->node = dent->d_ino;
			rec->pdev = dent->d_pdev;
			rec->pnode = dent->d_pino;
			rec->status = B_OK;
			rec->name = strdup(dent->d_name);
			rec->item = NULL;
			rec->state = B_OK;
			NewEntry(rec);
		}
		
		// unlock before getting more dirents
		Unlock();
	}

	//enable the button 'cause we are done
	Lock();
	fOKToQuit = true;
	button->SetEnabled(true);
	fThreadID = -1;
	Unlock();
}

void 
LiveQueryWindow::ClearResults()
{
	//clear the query
	fQuery->Clear();
	
	//empty the list view
	/* we have pointers to all of the items */
	/* so we don't delete them here */
	if (fValidView->IsEmpty() == false) fValidView->MakeEmpty();
		
	fTrackingLock.Lock();
	
	//clear the tracking list
	while (fValidList.IsEmpty() == false) {
		live_ref *ref = (live_ref *) fValidList.RemoveItem((int32) 0);
		DeleteEntry(ref);
	}

	//clear the zombie list
	while (fZombieList.IsEmpty() == false) {
		live_ref *ref = (live_ref *) fZombieList.RemoveItem((int32) 0);
		DeleteEntry(ref);
	}

	fTrackingLock.Unlock();
	
	UpdateCount();
}

void 
LiveQueryWindow::EntryCreated(BMessage *msg)
{
	ASSERT(IsLocked());

	/* a new item defined in msg now matches the query */
	printf("EntryCreated called\n");	

	live_ref *rec = NULL;

	//search for this ref
	node_ref node;
	msg->FindInt32("device", &node.device);
	msg->FindInt64("node", &node.node);
	const char *name = NULL;
	ino_t dir;
	msg->FindString("name", &name);
	msg->FindInt64("directory", &dir);
	
	if (FindCachedEntry(node, &rec, &fValidList) == B_OK) {
		//update the item
		printf("already in ValidList\n");
		if (strcmp(rec->name, name) != 0) {
			free(rec->name);
			rec->name = strdup(name);
		}
		if (rec->pnode != dir) rec->pnode = dir;

		rec->state = B_OK;

		UpdateEntry(rec, true);		

	} else if (FindCachedEntry(node, &rec, &fZombieList) == B_OK) {
		//update the item
		//move the item to the valid list if appropriate
		printf("already in ZombieList\n");
		if (strcmp(rec->name, name) != 0) {
			free(rec->name);
			rec->name = strdup(name);
		}
		if (rec->pnode != dir) rec->pnode = dir;
		rec->state = B_OK;
		
		UpdateEntry(rec, false);

	} else {	
		//create a new live ref
		rec = (live_ref *) malloc(sizeof(live_ref));
		
		const char *name;
		rec->dev = node.device;
		//not sure about this one
		rec->pdev = node.device;
		rec->node = node.node;
		msg->FindInt64("directory", &rec->pnode);
		msg->FindString("name", &name);
		rec->name = strdup(name);
		rec->status = B_OK;
		rec->item = NULL;
		rec->state = B_OK;
		NewEntry(rec);
	}
}

void 
LiveQueryWindow::EntryRemoved(BMessage *msg)
{
	ASSERT(IsLocked());

	/* the item defined in msg doesn't match query anymore */
	/* either because it no longer exists, or its name or mime-type */
	/* has changed */
	printf("EntryRemoved called\n");
	
	//get the node of the item
	node_ref node;
	if (msg->FindInt64("node", &node.node) != B_OK) {
		//we have a problem so let's just get out
		printf("invalid EntryRemoved msg\n");
		return;
	}
	
	if (msg->FindInt32("device", &node.device) != B_OK) {
		//we have a problem so let's just get out
		printf("invalid EntryRemoved msg\n");
		return;
	}
	
	live_ref *rec = NULL;
	if (FindCachedEntry(node, &rec, &fValidList) == B_OK) {
		rec->state = B_ENTRY_REMOVED;
		ValidToZombie(rec);
	} else if (FindCachedEntry(node, &rec, &fZombieList) == B_OK) {
		rec->state = B_ENTRY_REMOVED;
	} else {
		//we need to create the node and put it in the zombie list
		rec = (live_ref *) malloc(sizeof(live_ref));
		rec->node = node.node;
		rec->dev = node.device;
		rec->pdev = node.device;
		rec->pnode = 0; /*  */
		rec->name = NULL;
		rec->item = NULL;
		rec->status = B_OK;
		rec->state = B_ERROR;
		
		NewEntry(rec);
	}

	//set state to mark why we are not in the valid list
	rec->state = B_ENTRY_REMOVED;
	ValidToZombie(rec);
}

void 
LiveQueryWindow::EntryMoved(BMessage *msg)
{
	ASSERT(IsLocked());

	/* our entry has moved somewhere else, let us update our path */
	printf("B_NODE_MONITOR:B_ENTRY_MOVED\n");
	//unpack the bits and pieces
	node_ref node;
	ino_t opnode;
	ino_t npnode;
	/* find max name length */
	const char *name;

	msg->FindInt64("node", &node.node);
	msg->FindInt64("from directory", &opnode);
	msg->FindInt64("to directory", &npnode);
	msg->FindInt32("device", &node.device);
	msg->FindString("name", &name);
	
	live_ref * rec = NULL;
	if (FindCachedEntry(node, &rec, &fValidList) == B_OK) {
		//we have successfully found the item
		//let's see if we can find the actual entry
		printf("entry in valid list\n");
		
		//make changes
		if (rec->pnode != npnode) rec->pnode = npnode;
		if (strcmp(rec->name, name) != 0) {
			free(rec->name);
			rec->name = strdup(name);
		}
		
		UpdateEntry(rec, true);

	} else if (FindCachedEntry(node, &rec, &fZombieList) == B_OK) {
		//in the zombie list for some reason
		printf("entry in zombie list\n");
		
		//make changges
		if (rec->pnode != npnode) rec->pnode = npnode;
		if (strcmp(rec->name, name) != 0) {
			free(rec->name);
			rec->name = strdup(name);
		}
		
		UpdateEntry(rec, false);
		
	}
	else {
		//create a new live ref
		rec = (live_ref *) malloc(sizeof(live_ref));
		
		const char *name;
		rec->dev = node.device;
		//not sure about this one
		rec->pdev = node.device;
		rec->node = node.node;
		msg->FindInt64("directory", &rec->pnode);
		msg->FindString("name", &name);
		rec->name = strdup(name);
		rec->status = B_OK;
		rec->item = NULL;
		//set the state so that we know not to add it back to the list
		rec->state = B_ERROR;
		NewEntry(rec);
	}
	
}


void 
LiveQueryWindow::EntryChanged(int32 opcode, BMessage *msg)
{
	/* we want to update the entry because it has changed */
	/* this is where much of the meat difficulties in the */
	/* handling of live queries resides.  Depending on exactly */
	/* what kind of information we need about the entry in  */
	/* question and how up-to-date we need it to be, we have */
	/* various amounts of work to do here.  Essentially, the more */
	/* live your query needs to be and the more things you need to  */
	/* keep track of, the harder it is. */
	
	switch(opcode) {

		//something in the stat structure of the entry has changed
		//size, modification date etc.
		case B_STAT_CHANGED:
			printf("B_NODE_MONITOR:B_STAT_CHANGED\n");
			break;		
		
		//something in the attributes of the entry has changed
		case B_ATTR_CHANGED:
			printf("B_NODE_MONITOR:B_ATTR_CHANGED\n");
			break;		
		
		//a device has been mounted
		//note we do not care about mounting and unmounting devices
		case B_DEVICE_MOUNTED:
			break;		
		//a device has been unmounted
		//note we do not care about mounting and unmounting devices
		case B_DEVICE_UNMOUNTED:
			break;
			
		default:
			printf("unknown B_NODE_MONITOR opcode\n");
			break;
	}
}

void 
LiveQueryWindow::UpdateEntry(live_ref *rec, bool valid)
{
	ASSERT(IsLocked());

	entry_ref ref;
	ref.device = rec->pdev;
	ref.directory = rec->pnode;
	ref.set_name(rec->name);

	BEntry entry(&ref);
	if ((rec->status = entry.InitCheck()) == B_OK) {
		BPath path;
		entry.GetPath(&path);
		if ((rec->status = path.InitCheck()) == B_OK) {
			if (rec->item == NULL) rec->item = new BStringItem(path.Path());
			else if (strcmp(rec->item->Text(), path.Path()) != 0) {
				printf("new path: %s\n", path.Path());

				rec->item->SetText(path.Path());
				if (valid) 
					fValidView->InvalidateItem(fValidView->IndexOf(rec->item));
			}
		}
	}
	
	if (valid && rec->status != B_OK) ValidToZombie(rec);
	else if (valid == false && rec->status == B_OK && rec->state == B_OK)
		ZombieToValid(rec);
}

void 
LiveQueryWindow::NewEntry(live_ref *rec)
{
	ASSERT(IsLocked());

//	live_ref_to_stream(rec);

	/* start node-monitoring on this buffer */
	node_ref node;
	node.device = rec->dev;
	node.node = rec->node;
	
	/*status_t status = */
	 watch_node(&node, B_WATCH_NAME | B_WATCH_STAT | B_WATCH_ATTR, this);
	
//	printf("watch_node: %d\n", status);
	
	//let's see if we can find the actual entry
	entry_ref ref;
	ref.device = rec->pdev;
	ref.directory = rec->pnode;
	ref.set_name(rec->name);

	BEntry entry(&ref);
	if ((rec->status = entry.InitCheck()) == B_OK) {
		BPath path;
		entry.GetPath(&path);
		if ((rec->status = path.InitCheck()) == B_OK) {
			/* we now have all of the info we currently */
			/* care about.  if we needed more information */
			/* we could keep digging like get a BNode */
			/* for data or BNodeInfo for attributes */
			/* in any case, keep setting return results */
			/* into rec->status and when we look again */
			/* for update info we will have it */
			rec->item = new BStringItem(path.Path());
			//add the item to the view
			fValidView->AddItem(rec->item);
			UpdateCount();
		}
	}
	
	/* note that rec->status will either be B_OK */
	/* or will contain info regarding the status of the item */
	/* if you are looking for more info about an item than the path */
	/* you might want to add an item to the list view anyway */
	/* and mark it as having a little problem */
	
	//add the item to the tracking list or zombie list
	if (rec->status == B_OK && rec->state == B_OK) fValidList.AddItem(rec);
	else fZombieList.AddItem(rec);
}

void 
LiveQueryWindow::DeleteEntry(live_ref *rec)
{

	if (rec != NULL) {
		//make sure to stop monitoring node
		node_ref nref;
		nref.device = rec->dev;
		nref.node = rec->node;
		
		/* status_t status = */
		watch_node(&nref, B_STOP_WATCHING, this);
	//	printf("node_ref stop_watching: %d\n", status);
	
		//get rid of the BStringItem
		if (rec->item) {
			delete rec->item; rec->item = NULL;
		}
		//free the name
		free (rec->name);
		//free the item
		free (rec);
	}
}

status_t 
LiveQueryWindow::FindCachedEntry(node_ref node, live_ref **ref, BList *list)
{
	live_ref_ptr ptr;
	ptr.node = node;
	ptr.ref = NULL;

	fTrackingLock.Lock();
	
	list->DoForEach(FindLiveRef, &ptr);
	if (ptr.ref == NULL) {
		fTrackingLock.Unlock();
		return B_ENTRY_NOT_FOUND;
	}
	*ref = ptr.ref;
	
	fTrackingLock.Unlock();
	
	return B_OK;
}

bool 
LiveQueryWindow::FindLiveRef(void *item, void *rec)
{
//	printf("find cached item called\n");
	live_ref *ref = (live_ref *)item;
	live_ref_ptr *ptr = (live_ref_ptr *) rec;


	if ((ref->dev == ptr->node.device) && (ref->node == ptr->node.node)) {
//		printf("node match found!\n");
		ptr->ref = ref;
		return true;
	}
	ptr->ref = NULL;
	return false;	
}

void 
LiveQueryWindow::ValidToZombie(live_ref *ref)
{
	ASSERT(IsLocked());

	printf("moving ref from valid to zombie\n");
	//remove the item from the valid list
	if (ref->item) fValidView->RemoveItem(ref->item);
	UpdateCount();
	
	//move the item from the valid to the zombie list
	fTrackingLock.Lock();
	//we don't particularly care about the return value,
	//as we know we don;y want it in the valid list
	fValidList.RemoveItem(ref);
	
	if (!fZombieList.HasItem(ref)) {
		//if not already in the zombie list, add it
		fZombieList.AddItem(ref);
	}
	fTrackingLock.Unlock();
}

void 
LiveQueryWindow::ZombieToValid(live_ref *ref)
{
	ASSERT(IsLocked());
	
	if (ref) {
	
		printf("moving ref from zombie to valid\n");
		fTrackingLock.Lock();
		//we don't particularly care about the return value,
		//as we know we don't want it in the zombie list
		fZombieList.RemoveItem(ref);

		if (!fValidList.HasItem(ref)) {		
			//if not in the valid list, add it
			fValidList.AddItem(ref);
			fTrackingLock.Unlock();
			
			if (!fValidView->HasItem(ref->item)) {
				fValidView->AddItem(ref->item);
				UpdateCount();
			}
		} else {
			//be sure to unlock in any case
			fTrackingLock.Unlock();		
		}
	}
}

void 
LiveQueryWindow::UpdateCount()
{

	fCount = fValidView->CountItems();
	char buffer[32];
	sprintf(buffer, "Count: %ld", fCount);
	fCountString->SetText(buffer);
}

