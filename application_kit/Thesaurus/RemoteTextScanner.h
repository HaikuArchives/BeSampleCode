/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

//
//	RemoteTextScanner is a class that communicates with a text view in another
//	application using scripting.
//

#ifndef REMOTETEXTSCANNER_H
#define REMOTETEXTSCANNER_H

#include <String.h>
#include <Messenger.h>
#include <InterfaceDefs.h>

class Message;

const int32 TEXT_BUFFER_SIZE	= 0x1000;
const int32 MESSAGE_TIMEOUT		= 1000000;
const rgb_color HIGHLIGHT_COLOR = {255, 0, 0, 255};

class RemoteTextScanner {
public:

					RemoteTextScanner(const BMessenger&);
				
	// Sets the current word to be the next word in the text view, and
	// returns a pointer to it.
	const char*		GetNextWord();
	
	// Changes the selection of the text view to be the current word.
	// Note that the selection will not be visible if the window in
	// question is not in focus.
	status_t		SelectCurrentWord();
	
	// This changes the color of the current word, to the highlight color
	// if the parameter is true, or back to its previous value if the
	// parameter is false.  Generally the word should not be highlighted
	// unless this application has the focus.
	status_t		HighlightCurrentWord(bool);
	
	status_t		ReplaceCurrentWord(const char*);

	// Starts scanning from the beginning of the document.
	void			Rewind();	

	// Used to inform this scanner that the text in the targeted view may
	// have been modified.  This will invalidate its buffered text and start
	// scanning from the beginning of the last selected word.
	void 			TextModified();
	
private:

	status_t		FetchNextTextChunk();

	BMessenger 		fTextViewMessenger;

	BString			fCurrentWord;
	int32			fCurrentWordOffset;			// Offset from beginning of document

	int32			fNextCharacterOffset;		// Next character in buffer
	int32			fNextTextBufferStart;		// Offset to buffer from beginning of doc
	int32			fTextBufferLength;			// Current size of buffer
	char			fTextBuffer[TEXT_BUFFER_SIZE];

	rgb_color		fOriginalTextColor;
};

#endif