/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <AppKit.h>
#include <Debug.h>
#include <String.h>
#include <ctype.h>
#include <TextView.h>
#include "RemoteTextScanner.h"

RemoteTextScanner::RemoteTextScanner(const BMessenger &messenger)
	:	fTextViewMessenger(messenger),
		fCurrentWordOffset(0),
		fNextCharacterOffset(0),
		fNextTextBufferStart(0),
		fTextBufferLength(0)
{
}

void  
RemoteTextScanner::Rewind()
{
	fNextCharacterOffset = 0;
	fNextTextBufferStart = 0;
	fCurrentWordOffset = 0;
	fTextBufferLength = 0;
}

const char *
RemoteTextScanner::GetNextWord()
{
	fCurrentWordOffset += fCurrentWord.Length();
	fCurrentWord = "";
	
	enum {
		SCAN_NONTEXT,
		SCAN_TEXT
	} state = SCAN_NONTEXT;
	
	while (true) {
	
		// Fetch more text if necessary
		if (fNextCharacterOffset >= fTextBufferLength) {
			status_t err = FetchNextTextChunk();
			if (err != B_OK || fTextBufferLength == 0) {
				if (fCurrentWord == "")
					return 0;
				else
					return fCurrentWord.String();			
			}
		}
		
		char c = fTextBuffer[fNextCharacterOffset];
		
		switch (state) {
		case SCAN_NONTEXT:
			if (isalpha(c)) {
				state = SCAN_TEXT;
				continue;
			}

			fCurrentWordOffset++;
			break;

		case SCAN_TEXT:
			if (!isalpha(c))
				return fCurrentWord.String();

			fCurrentWord += c;		
			break;

		}

		fNextCharacterOffset++;
	}
	
	return 0;
}


status_t
RemoteTextScanner::SelectCurrentWord()
{
	BMessage setSelectionReply;
	BMessage setSelectionMsg(B_SET_PROPERTY);
	setSelectionMsg.AddInt32("data", fCurrentWordOffset);
	setSelectionMsg.AddInt32("data", fCurrentWordOffset + fCurrentWord.Length());
	setSelectionMsg.AddSpecifier("selection");

	return fTextViewMessenger.SendMessage(&setSelectionMsg,
		&setSelectionReply, MESSAGE_TIMEOUT, MESSAGE_TIMEOUT);
}



status_t 
RemoteTextScanner::HighlightCurrentWord(bool highlight)
{
	int start = fCurrentWordOffset;
	int length = fCurrentWord.Length();

	// Since the user can't see the selection, change the color of
	// the selected word.  First get the current text information.
	BMessage getTextRunsReply;
	BMessage getTextRunsMsg(B_GET_PROPERTY);
	getTextRunsMsg.AddSpecifier("text_run_array", start, length);
	status_t err = fTextViewMessenger.SendMessage(&getTextRunsMsg, &getTextRunsReply,
		MESSAGE_TIMEOUT, MESSAGE_TIMEOUT);
	if (err != B_OK)
		return err;

	text_run_array *currentSelection;
	int32 len;
	err = getTextRunsReply.FindData("result", B_RAW_TYPE, (const void**)
		&currentSelection, &len);
	if ( err != B_OK)
		return err;
		

	// Now change the color of all the runs in the selection
	if (highlight) {
		fOriginalTextColor = currentSelection->runs[0].color;
		for (int i = 0; i < currentSelection->count; i++)
			currentSelection->runs[i].color = HIGHLIGHT_COLOR;
	} else {
		// Note that this is a little lazy.  If the currently selected
		// word has multiple colors of text in it (which is a little
		// goofy anyways), when the color is restored, the entire
		// word will be the color of the first letter.  The proper solution
		// is to save all of the text runs. 
		for (int i = 0; i < currentSelection->count; i++)
			currentSelection->runs[i].color = fOriginalTextColor;
	}

	BMessage setTextRunsReply;
	BMessage setTextRunsMsg(B_SET_PROPERTY);
	setTextRunsMsg.AddData("data", B_RAW_TYPE, currentSelection, len);
	setTextRunsMsg.AddSpecifier("text_run_array", start, length);
	err = fTextViewMessenger.SendMessage(&setTextRunsMsg, &setTextRunsReply,
		MESSAGE_TIMEOUT, MESSAGE_TIMEOUT);

	return err;
}


status_t 
RemoteTextScanner::ReplaceCurrentWord(const char *newWord)
{
	// Erase the word that is going to be replaced
	BMessage reply;
	BMessage textDelMessage(B_SET_PROPERTY);
	textDelMessage.AddSpecifier("Text", fCurrentWordOffset, fCurrentWord.Length());
	status_t err = fTextViewMessenger.SendMessage(&textDelMessage,
		&reply, MESSAGE_TIMEOUT, MESSAGE_TIMEOUT);
	if (err != B_OK)
		return err;

	// Now insert the new word beginning at the same location
	BMessage textSetMessage(B_SET_PROPERTY);
	textSetMessage.AddString("data", newWord);
	textSetMessage.AddSpecifier("Text", fCurrentWordOffset, strlen(newWord));
	err = fTextViewMessenger.SendMessage(&textSetMessage, &reply, MESSAGE_TIMEOUT,
		MESSAGE_TIMEOUT);
	if (err != B_OK)
		return err;

	// Adjust the location of the buffer 
	fNextTextBufferStart += strlen(newWord) - fCurrentWord.Length();
	fCurrentWord = newWord; 
	return B_OK;
}


//
//	This method gets called when the text in the targeted view may have changed.
//	It resets the buffer to the beginning of the current word and starts scanning
//	from there.
//
void 
RemoteTextScanner::TextModified()
{
	fCurrentWord = "";
	fNextTextBufferStart = fCurrentWordOffset;
	fNextCharacterOffset = 0;
	fTextBufferLength = 0;
}


//
//	Grab another chunk of text.  This grabs text in large chunks because it is
// 	more efficient.  Returns true if it could grab a chunk, false if there
//	was no more.
//
status_t 
RemoteTextScanner::FetchNextTextChunk()
{
	fTextBufferLength = 0;

	BMessage reply;
	BMessage textRequestMessage(B_GET_PROPERTY);
	textRequestMessage.AddSpecifier("Text", fNextTextBufferStart, TEXT_BUFFER_SIZE);
	status_t err = fTextViewMessenger.SendMessage(&textRequestMessage, &reply,
		MESSAGE_TIMEOUT, MESSAGE_TIMEOUT);
	if (err != B_OK)
		return err;

	const char *text;
	err = reply.FindString("result", &text);
	if (err != B_OK)
		return err;

	strncpy(fTextBuffer, text, TEXT_BUFFER_SIZE);	
	fTextBufferLength = strlen(text);
	fNextTextBufferStart += fTextBufferLength;
	fNextCharacterOffset = 0;
	
	return B_OK;
}