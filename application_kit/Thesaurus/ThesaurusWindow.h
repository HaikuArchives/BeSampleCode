/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

//
//	Main application window
//
#ifndef THESAURUSWINDOW_H
#define THESAURUSWINDOW_H

#include <Window.h>
#include <ListView.h>
#include <Messenger.h>
#include <TextControl.h>
#include "Dictionary.h"

class RemoteTextScanner;
class DragSelector;

class ThesaurusWindow : public BWindow {
public:

						ThesaurusWindow();
						~ThesaurusWindow();
	void				SetMessenger(BMessenger);
	void				EnableChecking();
	void				StopScanning();

private:

	virtual bool		QuitRequested();
	virtual void		MessageReceived(BMessage*);
	virtual void		WindowActivated(bool);

	void				FindNextWord();

	BButton 			*fReplaceButton;
	BButton 			*fSkipButton;
	BButton 			*fCancelButton;
	BListView 			*fReplacementList;
	BTextControl		*fCurrentWord;
	DragSelector		*fDragger;

	RemoteTextScanner	*fScanner;
	BList				*fSynonymList;
	
	enum Case {
		LOWERCASE,
		CAPITALIZED,
		ALLCAPS
	} fCurrentWordCase;
	
	bool				fFoundWordOnLastPass;
};

#endif
