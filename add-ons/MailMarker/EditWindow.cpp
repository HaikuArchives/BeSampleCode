/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "EditWindow.h"

#include <Button.h>
#include <File.h>
#include <TextView.h>
#include <string.h>
#include <malloc.h>


const uint32 SAVE = '?sav';
const uint32 REVERT = '?rvt';

EditWindow::EditWindow(BFile *file, char **buffer)
	: 	BWindow(BRect(50,50,300, 300), "Status", B_TITLED_WINDOW, 0),
		fOrigin(buffer), fFile(file)
{
	
	fBuffer = strdup(*buffer);
	
	BRect rect = Bounds();
	rect.OffsetTo(B_ORIGIN);
	rect.bottom = rect.bottom -25;

	BRect text_rect = rect;
	text_rect.InsetBy(BPoint (5, 5));

	fText = new BTextView(rect, "Text", text_rect, B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
	fText->SetText(fBuffer);

	
	rect.top = rect.bottom;
	rect.bottom += 25;
	rect.right = 125;
	fRevert = new BButton(rect, "Revert", "Revert", new BMessage(REVERT), B_FOLLOW_BOTTOM);
	rect.right = 250;
	rect.left = 125;
	fSave = new BButton(rect, "Save", "Save", new BMessage(SAVE), B_FOLLOW_BOTTOM);
	
	AddChild(fRevert);
	fRevert->SetTarget(this);
	AddChild(fSave);
	fSave->SetTarget(this);

	AddChild(fText);

	Show();
}


EditWindow::~EditWindow()
{
	free(fBuffer);
}

void 
EditWindow::MessageReceived(BMessage *msg)
{
	if (msg->what == REVERT) {
		//set the text back to it's original
		fText->SetText(fBuffer);
	} else if (msg->what == SAVE) {
		free(fBuffer);
		const char *text = fText->Text();
		int32 text_len = strlen(text);
		if (text[text_len - 1] != '\n') {
			text_len++;
			fBuffer = (char *) malloc(text_len);
			strcpy(fBuffer, text);
			fBuffer[text_len -1 ] = '\n';
			fBuffer[text_len] = '\0';
		} else fBuffer = strdup(text);

		fFile->WriteAt(0, fBuffer, text_len);
		fFile->SetSize(text_len);
		
		free(*fOrigin);
		*fOrigin = strdup(fBuffer);
		
		PostMessage(B_QUIT_REQUESTED, this);
	} else BWindow::MessageReceived(msg);
}
