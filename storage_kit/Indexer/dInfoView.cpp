/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "dInfoView.h"

#include <posix/sys/stat.h>
#include <posix/string.h>
#include <posix/stdio.h>
#include <Directory.h>
#include <Path.h>
#include <List.h>
#include <Window.h>
#include <posix/malloc.h>
#include <time.h>

dInfoView::dInfoView(entry_ref &ref, struct stat &st, BDirectory &dir)
			: BView(infoRect, "", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS),
					fStatus(B_NO_ERROR), fRef(ref), fNameStr(NULL), fPathStr(NULL),
					fCreateStr(NULL), fModStr(NULL), fIndexList(NULL),
					fEntryCount(0), fSubDirCount(0), fLinkCount(0), fFileCount(0),
					fIndexed(0), fPartialIndexed(0), fNotIndexed(0), fInvalidCount(0)
{
	//set the view's background color
	SetViewColor(light);
	
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
	
	//Get the list of attribute indices
	fIndexList = new BList();
	get_attribute_indices(fRef.device, *fIndexList);

	//Traverse the directory	
	TraverseDirectory(dir);
}


dInfoView::~dInfoView()
{
	//free the various strings
	free(fNameStr); fNameStr = NULL;
	free(fPathStr); fPathStr = NULL;
	free(fCreateStr); fCreateStr = NULL;
	free(fModStr); fModStr = NULL;

	//free the attribute index list and it's items
	int32 zero = 0;
	if (fIndexList != NULL) {
		while (fIndexList->IsEmpty() == false) {
			char *string = (char *) fIndexList->RemoveItem(zero);
			free(string); string = NULL;
		}
		delete fIndexList; fIndexList = NULL;
	}
}

status_t 
dInfoView::InitCheck()
{
	return fStatus;
}


char *
dInfoView::GetName()
{
	return fNameStr;
}

void 
dInfoView::MessageReceived(BMessage *msg)
{
	//if B_REFS_RECEIVED or B_SIMPLE_DATA mark as B_REFS_RECEIVED and send to app
	if (msg->what == B_SIMPLE_DATA || msg->what == B_REFS_RECEIVED) {
		msg->what = B_REFS_RECEIVED;
		be_app->PostMessage(msg, be_app);
	}else BView::MessageReceived(msg);
}

/* for every entry in the directory pass it to EvaluateRef */
void 
dInfoView::TraverseDirectory(BDirectory &dir)
{
	entry_ref ref;
	while(dir.GetNextRef(&ref) != B_ENTRY_NOT_FOUND) {
		EvaluateRef(ref);
	}	
}

void 
dInfoView::EvaluateRef(entry_ref &ref)
{
	struct stat st;
	BEntry entry;
	
	if (entry.SetTo(&ref, false) != B_OK) return;		
	
	fEntryCount++;
	
	entry.GetStat(&st);

	/* if a symlink, count it */	
	if (S_ISLNK(st.st_mode)) {
		fLinkCount++;
	}
	
	/* if a file from our device reindex it*/
	else if (S_ISREG(st.st_mode)) {
		if (ref.device != fRef.device) {
			fInvalidCount++; return;
		}
		
		fFileCount++;
		
		BNode node;
		if (node.SetTo(&ref) != B_NO_ERROR) {
			fInvalidCount++; return;
		}
		
		status_t status = B_OK;
		status = reindex_node(node, *fIndexList);
	
		
		if (status == INDEXED) fIndexed++;
		else if (status == PARTIAL_INDEXED) fPartialIndexed++;
		else fNotIndexed++;		
	}
	
	/* if a directory transverse it */
	else if (S_ISDIR(st.st_mode)) {
		BDirectory dir;
		if (dir.SetTo(&ref) != B_OK) {
			fInvalidCount++;
			return;
		}
		if (dir.IsRootDirectory()) fInvalidCount++;
		
		else {
			fSubDirCount++;
			TraverseDirectory(dir);
		}
	}
}

void 
dInfoView::Draw(BRect updateRect)
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

	char str_buf[64];

	//start at the top left and down the large offset
	BPoint pt(0,large_offset);
	
	//Directory Name
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
	
	
	//Kind: Directory labels
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
	DrawString(lDir);
	
		
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

	//Entry Count
	pt.y += large_offset;

	SetFontSize(10);	
	SetHighColor(red);
	SetLowColor(medium);

	width = StringWidth(lEntries);
	pt.x = INFO_OFFSET - width -5;
	MovePenTo(pt);
	DrawString(lEntries);
	
	pt.x = INFO_OFFSET + 5;
	SetHighColor(black);
	SetLowColor(light);
	MovePenTo(pt);
	
	sprintf(str_buf, "%ld", fEntryCount);
	DrawString(str_buf);
	
	
	//File Count
	pt.y += large_offset;

	SetFontSize(9);	
	SetHighColor(red);
	SetLowColor(medium);

	width = StringWidth(lFiles);
	pt.x = INFO_OFFSET - width -5;
	MovePenTo(pt);
	DrawString(lFiles);
		
	pt.x = INFO_OFFSET + 5;
	SetHighColor(black);
	SetLowColor(light);
	MovePenTo(pt);
	
	sprintf(str_buf, "%ld (%ld indexed- %ld partial - %ld not indexed)",
				fFileCount, fIndexed, fPartialIndexed, fNotIndexed);
	DrawString(str_buf);

	//Link Count
	pt.y += small_offset;
	
	SetHighColor(red);
	SetLowColor(medium);

	width = StringWidth(lLinks);
	pt.x = INFO_OFFSET - width -5;
	MovePenTo(pt);
	DrawString(lLinks);
	
	pt.x = INFO_OFFSET + 5;
	SetHighColor(black);
	SetLowColor(light);
	MovePenTo(pt);
	
	sprintf(str_buf, "%ld", fLinkCount);
	DrawString(str_buf);
	
	//Sub Directory Count
	pt.y += small_offset;
	
	SetHighColor(red);
	SetLowColor(medium);

	width = StringWidth(lSubDir);
	pt.x = INFO_OFFSET - width -5;
	MovePenTo(pt);
	DrawString(lSubDir);
	
	pt.x = INFO_OFFSET + 5;
	SetHighColor(black);
	SetLowColor(light);
	MovePenTo(pt);
	
	sprintf(str_buf, "%ld", fSubDirCount);
	DrawString(str_buf);
		
	//Invalid Entries count
	pt.y += small_offset;
	
	SetHighColor(red);
	SetLowColor(medium);

	width = StringWidth(lInvalid);
	pt.x = INFO_OFFSET - width -5;
	MovePenTo(pt);
	DrawString(lInvalid);
	
	pt.x = INFO_OFFSET + 5;
	SetHighColor(black);
	SetLowColor(light);
	MovePenTo(pt);
	
	sprintf(str_buf, "%ld", fInvalidCount);
	DrawString(str_buf);

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


	pt.x = INFO_OFFSET + 5 + max + 10;
	rect = Bounds();
	
	float x = 0;
	
	if (!rect.Contains(pt)) {
		x = pt.x - rect.right;
	}

	pt.y += 10;
	
	Window()->ResizeBy(x,  (pt.y - rect.bottom));
		
}

