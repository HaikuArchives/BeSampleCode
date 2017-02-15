/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "lInfoView.h"

#include <Window.h>
#include <posix/sys/stat.h>
#include <SymLink.h>
#include <Path.h>
#include <posix/string.h>
#include <posix/stdio.h>
#include <posix/malloc.h>
#include <time.h>

lInfoView::lInfoView(entry_ref &ref, struct stat &st)
			: BView(infoRect, "", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS),
				fStatus(B_OK), fRef(ref), fNameStr(NULL), fPathStr(NULL),
				fCreateStr(NULL), fModStr(NULL), fLinkStr(NULL)
{
	//set the view's background color
	SetViewColor(light);

	BEntry entry;
	fStatus = entry.SetTo(&ref);

	if (fStatus != B_OK) return;
	
	//get the path to the entry
	BPath path;
	fStatus = entry.GetPath(&path);
	
	if (fStatus != B_OK) return;

	//build the symlink
	BSymLink link;
	fStatus = link.SetTo(path.Path());
	
	if (fStatus != B_OK) return;
	
	fAbsolute = link.IsAbsolute();

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
	
	//read the link string
	char str_buf[B_PATH_NAME_LENGTH];
	str_buf[0] = '\0';
	ssize_t size = link.ReadLink(str_buf, B_PATH_NAME_LENGTH);

	if (size >= 0) fLinkStr = strdup(str_buf);
	
}


lInfoView::~lInfoView()
{
	//free the various strings
	free(fNameStr); fNameStr = NULL;
	free(fPathStr); fPathStr = NULL;
	free(fCreateStr); fCreateStr = NULL;
	free(fModStr); fModStr = NULL;
	free(fLinkStr); fLinkStr = NULL;
}

status_t 
lInfoView::InitCheck()
{
	return fStatus;
}

char *
lInfoView::GetName()
{
	return fNameStr;
}

void 
lInfoView::MessageReceived(BMessage *msg)
{
	//if B_REFS_RECEIVED or B_SIMPLE_DATA mark as B_REFS_RECEIVED and send to app
	if (msg->what == B_SIMPLE_DATA || msg->what == B_REFS_RECEIVED) {
		msg->what = B_REFS_RECEIVED;
		be_app->PostMessage(msg, be_app);
	}else BView::MessageReceived(msg);
}

void 
lInfoView::Draw(BRect updateRect)
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
	
	//Link Name
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
	
	//Kind: Link labels
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
	DrawString(lLink);
	
		
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

	//Type
	pt.y += large_offset;
	
	SetHighColor(red);
	SetLowColor(medium);

	width = StringWidth(lType);
	pt.x = INFO_OFFSET - width -5;
	MovePenTo(pt);
	DrawString(lType);
	
	pt.x = INFO_OFFSET + 5;
	SetHighColor(black);
	SetLowColor(light);
	MovePenTo(pt);
	if (fAbsolute == true) DrawString(lAbsolute);
	else DrawString(lRelative);
	
	//Link To
	pt.y += large_offset;
	
	SetHighColor(red);
	SetLowColor(medium);

	width = StringWidth(lLinkTo);
	pt.x = INFO_OFFSET - width -5;
	MovePenTo(pt);
	DrawString(lLinkTo);
	
	pt.x = INFO_OFFSET + 5;
	SetHighColor(black);
	SetLowColor(light);
	MovePenTo(pt);
	DrawString(fLinkStr);

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

	width = StringWidth(fLinkStr);
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

