/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "fInfoView.h"

#include <posix/sys/stat.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Path.h>
#include <posix/string.h>
#include <posix/stdio.h>
#include <List.h>
#include <ListView.h>
#include <ListItem.h>
#include <Window.h>
#include <Entry.h>
#include <posix/malloc.h>
#include <time.h>

fInfoView::fInfoView(entry_ref &ref, struct stat &st)
			: BView(infoRect, "", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS),
				fRef(ref), fAttrList(NULL), fIndexList(NULL), fStatus(B_NO_ERROR),
				fNameStr(NULL), fCreateStr(NULL), fModStr(NULL), fPathStr(NULL),
				fSizeStr(NULL), fMimeStr(NULL)
{
	//set the view's background color
	SetViewColor(light);
	
	//get the node
	BNode node;
	fStatus = node.SetTo(&ref);
	if (fStatus != B_NO_ERROR) return;
	
	BEntry entry;
	fStatus = entry.SetTo(&ref);

	if (fStatus != B_NO_ERROR) return;
	
	//get the path to the entry
	BPath path;
	fStatus = entry.GetPath(&path);
	
	if (fStatus != B_NO_ERROR) return;
	
	//get the Path from path
	fPathStr = strdup(path.Path());

	//get the name from the ref	
	fNameStr = strdup(fRef.name);

	//get Creation and Modification times out of the stat
	char buf[32];
	
	tm *timep = localtime(&st.st_mtime);
	strftime(buf, 32, "%a, %b %d %Y, %I:%M %p", timep);

	fModStr = strdup(buf);
	
	timep = localtime(&st.st_ctime);
	strftime(buf, 32, "%a, %b %d %Y, %I:%M %p", timep);
	
	fCreateStr = strdup(buf);

	//Size String	
	sprintf(buf, "%Ld bytes", st.st_size);
	fSizeStr = strdup(buf);
	
	//get the indexed attributes
	fIndexList = new BList();
	get_attribute_indices(fRef.device, *fIndexList);
	
	fIndexed = reindex_node(node, *fIndexList);

	//get the file's attributes
	BList list;	
	GetAttributes(node, list);
	
	//if it has attributes create the list view	
	if (list.CountItems() > 0) {
		BRect frame = Bounds();
		frame.InsetBy(10,10);
		frame.left = frame.left + INFO_OFFSET;
		frame.top = frame.top + 75;
		
		
		fAttrList = new BListView(frame, "AttrList");

		list.DoForEach(make_view_items, fAttrList);

		fAttrList->SetFontSize(9);

		AddChild(fAttrList);		
	}
	
	
	//be sure to free list and it's contents
	int32 zero = 0;
	while (list.IsEmpty() == false) {
		char *string = (char *) list.RemoveItem(zero);
		free(string); string = NULL;
	}	
	
	//Get the Mime Type data
	BNodeInfo node_info;
	if (node_info.SetTo(&node) == B_NO_ERROR) {
		char type[64];
		if (node_info.GetType(type) == B_NO_ERROR) {
			fMimeStr = strdup(type);
		}
	}
	
}


fInfoView::~fInfoView()
{
	//free the strings
	free(fNameStr); fNameStr = NULL;
	free(fCreateStr); fCreateStr = NULL;
	free(fModStr); fModStr = NULL;
	free(fPathStr); fPathStr = NULL;
	free(fSizeStr); fSizeStr = NULL;
	free(fMimeStr); fMimeStr = NULL;
	
	//free the indexed attribute list and its contents
	int32 zero = 0;
	if (fIndexList != NULL) {
		while (fIndexList->IsEmpty() == false) {
			char *string = (char *) fIndexList->RemoveItem(zero);
			free(string); string = NULL;
		}
		delete fIndexList; fIndexList = NULL;
	}

	//free the list view and its contents
	if (fAttrList != NULL) {
		while (fAttrList->IsEmpty() == false) {
			BStringItem *item = (BStringItem *) fAttrList->RemoveItem(zero);
			delete item; item = NULL;
		}
		delete fAttrList; fAttrList = NULL;
	}
	
}

status_t 
fInfoView::InitCheck()
{
	return fStatus;
}


char *
fInfoView::GetName()
{
	return fNameStr;
}

void 
fInfoView::MessageReceived(BMessage *msg)
{
	//if B_REFS_RECEIVED or B_SIMPLE_DATA mark as B_REFS_RECEIVED and send to app
	if (msg->what == B_SIMPLE_DATA || msg->what == B_REFS_RECEIVED) {
		msg->what = B_REFS_RECEIVED;
		be_app->PostMessage(msg, be_app);
	
	
	}else BView::MessageReceived(msg);
}

//get the file's attributes
void 
fInfoView::GetAttributes(BNode &node, BList &list)
{
	char attr_buf[B_ATTR_NAME_LENGTH];
	
	node.RewindAttrs();
	
	while (node.GetNextAttrName(attr_buf) == B_NO_ERROR) {
		char *string = strdup(attr_buf);
		list.AddItem(string);
	}
}

void 
fInfoView::Draw(BRect updateRect)
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
	
	//File Name
	pt.x = INFO_OFFSET + 5;
	SetHighColor(black);
	SetLowColor(light);
	MovePenTo(pt);
	DrawString(fNameStr);
	
	SetFontSize(9);
	GetFontHeight(&fht);
//	float small_offset = fht.ascent + fht.descent + fht.leading;

	float width = StringWidth(lName);
	pt.x = INFO_OFFSET - width -5;
	SetHighColor(red);
	SetLowColor(medium);
	MovePenTo(pt);
	DrawString(lName);
	
	
	//Kind: File labels
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
	DrawString(lFile);
	
		
	//Path
	pt.y += large_offset;
	
	SetHighColor(red);
	SetLowColor(medium);
	
	width = StringWidth(lPath);
	pt.x = INFO_OFFSET - width -5;
	MovePenTo(pt);
	DrawString(lPath);
	
	pt.x = INFO_OFFSET + 5;
	SetHighColor(black);
	SetLowColor(light);
	MovePenTo(pt);
	DrawString(fPathStr);
	
	
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

	//Mime
	if (fMimeStr != NULL) {
		pt.y += large_offset;	
		
		SetHighColor(red);
		SetLowColor(medium);
		
		width = StringWidth(lMime);
		pt.x = INFO_OFFSET - width -5;
		MovePenTo(pt);
		DrawString(lMime);
	
		pt.x = INFO_OFFSET + 5;
		SetHighColor(black);
		SetLowColor(light);
		MovePenTo(pt);
		DrawString(fMimeStr);
	}

	//Status
	pt.y += large_offset;	
	
	SetHighColor(red);
	SetLowColor(medium);
	
	width = StringWidth(lStatus);
	pt.x = INFO_OFFSET - width -5;
	MovePenTo(pt);
	DrawString(lStatus);

	pt.x = INFO_OFFSET + 5;
	SetHighColor(black);
	SetLowColor(light);
	MovePenTo(pt);
	
	if (fIndexed == INDEXED) DrawString(lIndexed);
	else if (fIndexed == PARTIAL_INDEXED) DrawString(lPartialIndexed);
	else DrawString(lNotIndexed);

		
	float bottom = 0;
	//Atribute List
	pt.y += large_offset;	
	
	if (fAttrList != NULL) {
		
		SetHighColor(red);
		SetLowColor(medium);
	
		width = StringWidth(lAttr);
		pt.x = INFO_OFFSET - width - 5;
		MovePenTo(pt);
		DrawString(lAttr);
		
		pt.x = INFO_OFFSET + 5;
		pt.y -= fht.ascent;
		fAttrList->MoveTo(pt);
		
		BRect list_rect = fAttrList->Frame();
		
		bottom = rect.bottom;
		
		fAttrList->ResizeBy(0, bottom - list_rect.bottom - 10);
	}

	//make sure we are wide enough
	float max = 0;
	width = StringWidth(fNameStr);
	if (width > max) max = width;	

	width = StringWidth(fCreateStr);
	if (width > max) max = width;	
		
	width = StringWidth(fModStr);
	if (width > max) max = width;	

	width = StringWidth(fPathStr);
	if (width > max) max = width;	

	width = StringWidth(fSizeStr);
	if (width > max) max = width;	

	width = StringWidth(fMimeStr);
	if (width > max) max = width;	


	pt.x = INFO_OFFSET + 5 + max + 10;
	rect = Bounds();
	
	float x = 0;
	
	if (!rect.Contains(pt)) {
		x = pt.x - rect.right;
	}

	pt.y = bottom;
	
	Window()->ResizeBy(x, (pt.y - rect.bottom));
}
