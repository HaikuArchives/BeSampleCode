/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "vInfoView.h"
#include <posix/string.h>
#include <posix/stdio.h>
#include <posix/sys/stat.h>
#include <Volume.h>
#include <List.h>
#include <ListView.h>
#include <ListItem.h>
#include <Path.h>
#include <Entry.h>
#include <Directory.h>
#include <Window.h>
#include <be/kernel/fs_index.h>
#include <posix/malloc.h>
#include <time.h>

vInfoView::vInfoView(BVolume &vol, struct stat &st)
		: BView(infoRect, "", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS),
			fDevice(vol.Device()), fStatus(B_NO_ERROR), fIndexList(NULL),
			fNameStr(NULL), fMountPoint(NULL), fCreateStr(NULL), fModStr(NULL),
			fSizeStr(NULL), fVolumeFlags(0)
{
	//set the view's background color
	SetViewColor(light);

	//get Creation and Modification times out of the stat
	char buf[32];
	
	tm *timep = localtime(&st.st_mtime);
	strftime(buf, 32, "%a, %b %d %Y, %I:%M %p", timep);
	fModStr = strdup(buf);
	
	timep = localtime(&st.st_ctime);
	strftime(buf, 32, "%a, %b %d %Y, %I:%M %p", timep);
	fCreateStr = strdup(buf);

		
	char str_buf[256];

	//get the Size string	
	off_t capacity = vol.Capacity();
	off_t bytes_free = vol.FreeBytes();
	
	sprintf(str_buf, "%Ld bytes (%Ld free)", capacity, bytes_free);
	
	fSizeStr = strdup(str_buf);

	//get the path to the root directory as the Mount Point
	//get the root dir
	BDirectory dir;
	vol.GetRootDirectory(&dir);
	//get it's entry	
	BEntry entry;
	dir.GetEntry(&entry);
	//get the path for that entry
	BPath path;
	entry.GetPath(&path);
	//set it as the Mount Point	
	fMountPoint = strdup(path.Path());

	//get the Name of the Volume
	// if the volume itself has no name, use the mount point as the name
	vol.GetName(str_buf);
	if (str_buf[0] != '\0') fNameStr = strdup(str_buf);
	else fNameStr = strdup(fMountPoint);
	
	//fVolume flags is used as a bit field of information about the volume
	//set each field of the bitfield as appropriate
	if (vol.IsRemovable()) fVolumeFlags |= IS_REMOVE;
	if (vol.IsReadOnly()) fVolumeFlags |= IS_RDONLY;
	if (vol.IsPersistent()) fVolumeFlags |= IS_PERSIST;
	if (vol.IsShared()) fVolumeFlags |= IS_SHARED;
	if (vol.KnowsMime()) fVolumeFlags |= IS_MIME;
	if (vol.KnowsQuery()) fVolumeFlags |= IS_QUERY;
	if (vol.KnowsAttr()) fVolumeFlags |= IS_ATTR;

	//if it understands queries and attributes get the names of the
	//indexed attributes and put them in fIndexList
	if (fVolumeFlags & IS_QUERY && fVolumeFlags & IS_ATTR) {
		
		BRect frame = Bounds();
		frame.InsetBy(10,10);
		frame.left = frame.left + INFO_OFFSET;
		frame.top = frame.top + 75;
		
		fIndexList = new BListView(frame, "IndexList");
		
		BList list;
		status_t status = get_attribute_indices(fDevice, list);
		if (status == B_NO_ERROR) {
			list.DoForEach(make_view_items, fIndexList);
		}
		
		fIndexList->SetFontSize(9);
		
		AddChild(fIndexList);
		
		int32 zero = 0;
		
		//make sure and delete all of the items in list
		while (list.IsEmpty() == false) {
			char *string = (char *) list.RemoveItem(zero);
			free(string); string = NULL;
		}	
	}
}


vInfoView::~vInfoView()
{
	//free the various strings
	free(fNameStr); fNameStr = NULL;
	free(fMountPoint); fMountPoint = NULL;
	free(fCreateStr); fCreateStr = NULL;
	free(fModStr); fModStr = NULL;
	free(fSizeStr); fSizeStr = NULL;
	
	//free the attribute index list and it's items
	int32 zero = 0;
	if (fIndexList != NULL) {
		while(fIndexList->IsEmpty() == false) {
			BStringItem *item = (BStringItem *) fIndexList->RemoveItem(zero);
			delete item; item = NULL;	
		}
		delete fIndexList; fIndexList = NULL;
	}

}

status_t 
vInfoView::InitCheck() const
{
	return fStatus;
}


char *
vInfoView::GetName() const
{
	return fNameStr;
}

void 
vInfoView::MessageReceived(BMessage *msg)
{
	if (msg->what == B_SIMPLE_DATA || msg->what == B_REFS_RECEIVED) {
		msg->what = B_REFS_RECEIVED;
		be_app->PostMessage(msg, be_app);
	}else BView::MessageReceived(msg);
}

void 
vInfoView::Draw(BRect updateRect)
{
	//draw the darker rectangle
	BRect rect = Bounds();
	rect.right = rect.left + INFO_OFFSET;
	
	SetHighColor(medium);
	SetLowColor(medium);
	
	FillRect(rect);
	
	//draw the separator line
	SetHighColor(dark);
	StrokeLine(rect.RightTop(), rect.RightBottom());


	//get font height information so we know the appropriate offsets
	font_height fht;
	SetFontSize(14);
	GetFontHeight(&fht);
	float large_offset = fht.ascent + fht.descent + fht.leading;
	
	//start at the top left and down the large offset
	BPoint pt(0,large_offset);
	
	//Volume Name
	pt.x = INFO_OFFSET + 5;
	SetHighColor(black);
	SetLowColor(light);
	MovePenTo(pt);
	DrawString(fNameStr);
	
	SetFontSize(9);
	GetFontHeight(&fht);
	float small_offset = fht.ascent + fht.descent + fht.leading;

	float width = StringWidth(lName);
	pt.x = INFO_OFFSET - width -5;
	SetHighColor(red);
	SetLowColor(medium);
	MovePenTo(pt);
	DrawString(lName);

	
	//Kind: Volume labels
	pt.y += large_offset;
	
	SetHighColor(red);
	SetLowColor(medium);

	width = StringWidth(lKind);
	pt.x = INFO_OFFSET - width -5;
	MovePenTo(pt);
	DrawString(lKind);
	
	pt.x = INFO_OFFSET + 5;
	SetHighColor(black);
	SetLowColor(light);
	MovePenTo(pt);
	DrawString(lVol);


	//Mount Point
	pt.y += large_offset;
	
	SetHighColor(red);
	SetLowColor(medium);

	width = StringWidth(lMountPt);
	pt.x = INFO_OFFSET - width -5;
	MovePenTo(pt);
	DrawString(lMountPt);
	
	pt.x = INFO_OFFSET + 5;
	SetHighColor(black);
	SetLowColor(light);
	MovePenTo(pt);
	DrawString(fMountPoint);
	
	//Created 
	pt.y += large_offset;
	
	SetHighColor(red);
	SetLowColor(medium);

	width = StringWidth(lCreated);
	pt.x = INFO_OFFSET - width -5;
	MovePenTo(pt);
	DrawString(lCreated);
	
	pt.x = INFO_OFFSET + 5;
	SetHighColor(black);
	SetLowColor(light);
	MovePenTo(pt);
	DrawString(fCreateStr);

	//Modified
	pt.y += large_offset;
	
	SetHighColor(red);
	SetLowColor(medium);

	width = StringWidth(lMod);
	pt.x = INFO_OFFSET - width -5;
	MovePenTo(pt);
	DrawString(lMod);
	
	pt.x = INFO_OFFSET + 5;
	SetHighColor(black);
	SetLowColor(light);
	MovePenTo(pt);
	DrawString(fModStr);
	
	//Size
	pt.y += large_offset;	
	
	SetHighColor(red);
	SetLowColor(medium);
	
	width = StringWidth(lSize);
	pt.x = INFO_OFFSET - width -5;
	MovePenTo(pt);
	DrawString(lSize);

	pt.x = INFO_OFFSET + 5;
	SetHighColor(black);
	SetLowColor(light);
	MovePenTo(pt);
	DrawString(fSizeStr);
	
	//Volume Flags
	pt.y += large_offset;
	
	SetHighColor(red);
	SetLowColor(medium);
	width = StringWidth(lVolFlags);
	
	pt.x = INFO_OFFSET - width -5;
	MovePenTo(pt);
	DrawString(lVolFlags);
	
	SetHighColor(black);
	SetLowColor(light);
	pt.x = INFO_OFFSET + 5;
	MovePenTo(pt);
	
	
	//Write the flags
	//persistant or virtual
	if (fVolumeFlags & IS_PERSIST) {
		DrawString(Persistent);
		pt.y += small_offset;
		MovePenTo(pt);
	} else {
		DrawString(Virtual);
		pt.y += small_offset;
		MovePenTo(pt);
	}
	//read only
	if (fVolumeFlags & IS_RDONLY) {
		DrawString(ReadOnly);
		pt.y += small_offset;
		MovePenTo(pt);
	}
	//removable
	if (fVolumeFlags & IS_REMOVE) {
		DrawString(Removable);
		pt.y += small_offset;
		MovePenTo(pt);
	}
	//shared
 	if (fVolumeFlags & IS_SHARED) {
		DrawString(Shared);
		pt.y += small_offset;
		MovePenTo(pt);
	}
	//adjust up to make sure there is a large offset
	//between each major item
	pt.y -= small_offset;

	//Awareness	
	pt.y += large_offset;
	
	SetHighColor(red);
	SetLowColor(medium);
	
	width = StringWidth(lAware);
	pt.x = INFO_OFFSET - width - 5;
	MovePenTo(pt);
	DrawString(lAware);
	
	SetHighColor(black);
	SetLowColor(light);
	pt.x = INFO_OFFSET + 5;
	MovePenTo(pt);

	//write the awareness flags
	bool flag_present = false;
	//Knows Attributes
	if (fVolumeFlags & IS_ATTR) {
		flag_present = true;
		DrawString(KnowsAttr);
		pt.y += small_offset;
		MovePenTo(pt);
	}
	//Knows Mime
	if (fVolumeFlags |= IS_MIME) {
		flag_present = true;
		DrawString(KnowsMime);
		pt.y += small_offset;
		MovePenTo(pt);
	}
	//Knows Query
	if (fVolumeFlags & IS_QUERY) {
		flag_present = true;
		DrawString(KnowsQuery);
		pt.y += small_offset;
		MovePenTo(pt);
	}
	//undo the last offset to make sure that there is a large
	//offset between each major item
	if (flag_present == true) pt.y -= small_offset;
	
	//Attribute Index list
	pt.y += large_offset;

	rect = Bounds();

	if (fIndexList != NULL) {
		
		SetHighColor(red);
		SetLowColor(medium);
	
		width = StringWidth(lIndexed);
		pt.x = INFO_OFFSET - width - 5;
		MovePenTo(pt);
		DrawString(lIndexed);
		
		pt.y += small_offset;
	
		width = StringWidth(lAttr);
		pt.x = INFO_OFFSET - width - 5;
		MovePenTo(pt);
		DrawString(lAttr);
	
		pt.y -= (small_offset + fht.ascent);
		pt.x = INFO_OFFSET + 5;
		//move the list to the proper place
		fIndexList->MoveTo(pt);
		
		//resize the list to make it fit in the view
		BRect list_rect = fIndexList->Frame();
		fIndexList->ResizeBy(0, rect.bottom - list_rect.bottom - 10);
	} else {
		//resize the view and the window to this size
		Window()->ResizeBy(0, -(rect.bottom - pt.y));
	}
	
	
}
