/* MailMarker Tracker Add-on */
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <TrackerAddOn.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <Node.h>
#include <Entry.h>
#include <string.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <Path.h>
#include <File.h>
#include <malloc.h>
#include "EditWindow.h"


const char defaults[] = "New\nSaved\nRead\nForwarded\nInfo\n";

void GenerateMenuItems(BPopUpMenu *popup, char *buffer, size_t length);
void EditSettings(BFile *file, BPopUpMenu *popup, char **buffer);

extern "C" void 
process_refs(entry_ref dir_ref, BMessage *msg, void *)
{
	//find the settings directory - if not found bail as there is
	//something seriously wrong
	BPath dir_path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &dir_path) != B_OK) return;

	
	BDirectory dir(dir_path.Path());
	if (dir.InitCheck() != B_OK) return;
	
	//look for the settings file
	BFile file;
	BEntry entry;
	dir.FindEntry("MailMarkerSettings", &entry);

	//if not write one
	if (entry.InitCheck() != B_OK) {
		dir.CreateFile("MailMarkerSettings", &file, true);
		if (file.InitCheck() == B_OK) {
			//write down the basics
			file.Write(defaults, strlen(defaults));
		}
	} else file.SetTo(&entry, B_READ_WRITE);
	
	char *buffer;
	if (file.InitCheck() == B_OK) {
		off_t file_size;
		file.GetSize(&file_size);
		buffer = (char *) malloc(file_size);
		ssize_t written = file.ReadAt(0, buffer, file_size);
		buffer[written] = '\0';
	} else buffer = strdup(defaults);
	
	
	
	//pop-up a menu listing the settings to set to
	// New, Saved, Read, Forwarded, Info
	BPopUpMenu * popup = new BPopUpMenu("Status");
	
	
	GenerateMenuItems(popup, buffer, strlen(buffer));
	
	
	BMenuItem * chosen = NULL;
	char *label = NULL;
	bool have_item = false; 
	while(!have_item) {
		chosen = popup->Go(BPoint(100,100), false, true);
		if (chosen) {
			label = strdup(chosen->Label());
			if (!strcmp(label, "Cancel")) {
				free(label);
				return;
			}else if (!strcmp(label, "Edit Settings")) {
				//adjust the file
				EditSettings(&file, popup, &buffer);
				//set the label to NULL so we loop again
				free(label);
				label = NULL;			
			} else have_item = true;
		} else have_item = true;
	}
	
	if (label) {
		int32 size = strlen(label) + 1;
		//stomp through each ref, check to see that it is an email and set it appropriately
		entry_ref ref;
		BNode node;
		char type[1024];
		for (int count = 0; msg->FindRef("refs", count, &ref) == B_NO_ERROR; count ++) {
			node.SetTo(&ref);
			if (node.InitCheck() == B_OK) {
				node.ReadAttr("BEOS:TYPE", B_STRING_TYPE, 0, type, 1024);
				if (!strcmp(type, "text/x-email")) {
					//we have an email - so set the status
					node.WriteAttr("MAIL:status", B_STRING_TYPE, 0, label, size);
				}
			}
		}
	}
	
	free(buffer);
	free(label);
	delete popup;

}

void
GenerateMenuItems(BPopUpMenu *popup, char *buffer, size_t length) {
	
	char *temp = strdup(buffer);
	
	//remove all current items
	while (popup->CountItems() > 0) {
		//remove the first item
		BMenuItem *item = popup->RemoveItem((int32) 0);
		delete item;
	}
	
	char *token = temp;
	BMenuItem *item = NULL;
	int32 len = strlen(temp);
	
	for (int32 i = 0; i < len; i++) {
		if (temp[i] == '\n') {
			temp[i] = '\0';
			item = new BMenuItem(token, NULL);
			popup->AddItem(item);	
			token = &temp[i + 1];
		}
	}

	BSeparatorItem *sep = new BSeparatorItem();
	popup->AddItem(sep);
	item = new BMenuItem("Cancel", NULL);
	popup->AddItem(item);
	item = new BMenuItem("Edit Settings", NULL);
	popup->AddItem(item);
	
	free(temp);
}

void
EditSettings(BFile *file, BPopUpMenu *popup, char **buffer) {
	//open a window to display the information	
	EditWindow *window = new EditWindow(file, buffer);

	thread_id thread = window->Thread();
	//wait for the window thread to return
	status_t win_status = B_OK;
	wait_for_thread(thread, &win_status);
	//regenerate the popup
	GenerateMenuItems(popup, *buffer, strlen(*buffer));
}

