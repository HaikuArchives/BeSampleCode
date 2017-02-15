/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <AppKit.h>
#include <InterfaceKit.h>
#include <SupportKit.h>
#include <ctype.h>
#include "ThesaurusWindow.h"
#include "RemoteTextScanner.h"
#include "Dictionary.h"
#include "DragSelector.h"


const int32 M_REPLACE_TEXT	= 'REPL';
const int32 M_SKIP_TEXT 	= 'SKIP';
const int32 M_CANCEL 		= 'CANC';

extern Dictionary dictionary;

ThesaurusWindow::ThesaurusWindow()
	:	BWindow(BRect(100, 100, 450, 300), "Thesaurus", B_TITLED_WINDOW_LOOK,
			B_NORMAL_WINDOW_FEEL, 0),
		fScanner(0),
		fFoundWordOnLastPass(false)
{
	SetZoomLimits(200, 800);
	SetSizeLimits(350, 550, 200, 900);

	// Background
	BBox *background = new BBox(Bounds(), "", B_FOLLOW_ALL_SIDES, B_WILL_DRAW |
		B_FRAME_EVENTS | B_NAVIGABLE_JUMP, B_PLAIN_BORDER);
	AddChild(background);
	background->SetViewColor(215, 215, 215);

	// List of replacements
	BRect replaceListRect(Bounds());
	replaceListRect.InsetBy(10, 10);
	replaceListRect.bottom -= (30 + B_H_SCROLL_BAR_HEIGHT);
	replaceListRect.right -= B_V_SCROLL_BAR_WIDTH;
	replaceListRect.top += 20;
	replaceListRect.left = 170;
	
	fReplacementList = new BListView(replaceListRect, "Replacement List",
		B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL_SIDES);
	BScrollView *replaceListScroller = new BScrollView("replace list scroller",
		fReplacementList, B_FOLLOW_ALL, 0, false, true);
	replaceListScroller->SetViewColor(215, 215, 215);
	fReplacementList->SetInvocationMessage(new BMessage(M_REPLACE_TEXT));
	background->AddChild(replaceListScroller);

	// Label for replacement list
	BRect replaceStringRect;
	replaceStringRect.left = replaceListRect.left;
	replaceStringRect.right = replaceListRect.right;
	replaceStringRect.top = 5;
	replaceStringRect.bottom = replaceListRect.top - 7;
	background->AddChild(new BStringView(replaceStringRect, "replace string",
		"Replace With"B_UTF8_ELLIPSIS));
	
	// Replace button
	BRect buttonRect(Bounds());
	buttonRect.InsetBy(10, 10);
	buttonRect.top = buttonRect.bottom - 25;
	buttonRect.left = buttonRect.right - 80;
	fReplaceButton = new BButton(buttonRect, "replace button", "Replace",
		new BMessage(M_REPLACE_TEXT), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	fReplaceButton->SetEnabled(false);
	fReplaceButton->MakeDefault(true);
	background->AddChild(fReplaceButton);

	// Skip button
	buttonRect.OffsetBy(-90, 0);
	fSkipButton = new BButton(buttonRect, "skip button", "Skip",
		new BMessage(M_SKIP_TEXT), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	fSkipButton->SetEnabled(false);
	background->AddChild(fSkipButton, fReplaceButton);

	// Cancel button
	buttonRect.OffsetBy(-110, 0);
	fCancelButton = new BButton(buttonRect, "cancel button", "Cancel",
		new BMessage(M_CANCEL), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	fCancelButton->SetEnabled(false);
	background->AddChild(fCancelButton, fSkipButton);
	
	// Text control for currently selected word
	BRect currentTextRect;
	currentTextRect.top = replaceListRect.top;
	currentTextRect.left = 10;
	currentTextRect.right = replaceListRect.left - 10;
	currentTextRect.bottom = currentTextRect.top + 25;
	fCurrentWord = new BTextControl(currentTextRect, "current text", "",
		"", 0);
	fCurrentWord->SetDivider(0);
	fCurrentWord->SetEnabled(false);
	background->AddChild(fCurrentWord);

	// Drag selector
	fDragger = new DragSelector(BRect(10, currentTextRect.bottom + 7, 42,
		currentTextRect.bottom + 49));
	background->AddChild(fDragger);
}

ThesaurusWindow::~ThesaurusWindow()
{
	delete fScanner;
}

void 
ThesaurusWindow::EnableChecking()
{
	Lock();
	fDragger->Enable();
	Unlock();
}


void 
ThesaurusWindow::SetMessenger(BMessenger msng)
{
	Lock();
	fDragger->Disable();
	if (fScanner == 0) {
		fScanner = new RemoteTextScanner(msng);
		FindNextWord();
	}
	Unlock();
}


void 
ThesaurusWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case M_REPLACE_TEXT: {
			int selection = fReplacementList->CurrentSelection(0);
			if (selection < 0)
				return ;		// Nothing Selected
				
			// Capitalize appropriately
			BString newString = ((Synonym*)fSynonymList->ItemAt(selection))->value;
			if (fCurrentWordCase == CAPITALIZED) {
				if (isalpha(newString[0]))
					newString[0] = toupper(newString[0]);
			} else if (fCurrentWordCase == ALLCAPS) {
				for (int i = 0; i < newString.Length(); i++)
					if (isalpha(newString[i]))
						newString[i] = toupper(newString[i]);
			}
			
			if (fScanner->ReplaceCurrentWord(newString.String()) == B_OK)
				FindNextWord();
			else
				StopScanning();
					
			break;
		}
	
		case M_SKIP_TEXT:
			FindNextWord();
			break;
			
		case M_CANCEL: {
			if (IsActive())
				fScanner->HighlightCurrentWord(false);

			StopScanning();
			break;
		}
				
		default:
			BWindow::MessageReceived(msg);
	}
}


//
//	There could be a problem if the user switches to the window that is being
//	checked and changes the text.  In that case, the buffered text stored in
//	the scanner will be invalid.  To prevent this problem, whenever we lose the
//	focus, we unhighlight the current word, and when we regain the focus, we
//	notify the scanner that the text might have been modified so it can re-read
//	the current area and try to find its place again.
//
void 
ThesaurusWindow::WindowActivated(bool isActive)
{
	if (isActive) {
		// Inform the scanner that the text may have been modified
		if (fScanner) {
			fScanner->TextModified();	
			FindNextWord();
		}
	} else {
		// Clear out selection
		if (fScanner)
			fScanner->HighlightCurrentWord(false);
	}
}


void 
ThesaurusWindow::FindNextWord()
{
	ASSERT(fScanner != 0);

	fReplaceButton->SetEnabled(false);
	fCancelButton->SetEnabled(false);
	fSkipButton->SetEnabled(false);

	// Clear out list
	for (int32 itemNum = fReplacementList->CountItems() - 1; itemNum >= 0;
		itemNum--)
		delete fReplacementList->RemoveItem(itemNum);

	
	if (IsActive())
		if (fScanner->HighlightCurrentWord(false) != B_OK) {
			StopScanning();
			return ;
		}

	while (true) {
		const char *word = fScanner->GetNextWord();
		if (word == 0) {
			// Reached end of document, try to start back at top
			if (!fFoundWordOnLastPass) {
				StopScanning();
				return ;	// Give Up.
			}

			fScanner->Rewind();
			fFoundWordOnLastPass = false;
			continue;
		}
		
		BString lowercase = word;
		for (int i = 0; i < lowercase.Length(); i++)
			if (isalpha(lowercase[i]))
				lowercase[i] = tolower(lowercase[i]);		
			
		fSynonymList = dictionary.FindAlternateWords(lowercase.String());

		if (fSynonymList) {
			fFoundWordOnLastPass = true;

			// Determine the case of the word
			fCurrentWordCase = LOWERCASE;
			if (word[0] >= 'A' && word[0] <= 'Z') {
				fCurrentWordCase = ALLCAPS;
				for (const char *i = word; *i; i++) {
					if (*i < 'A' || *i > 'Z') {
						fCurrentWordCase = CAPITALIZED;
						break;
					}
				}
			} 
		
			fCurrentWord->SetText(word);
		
			// Populate with fSynonymList for this word	
			for (int32 i = 0; i < fSynonymList->CountItems(); i++) {
				fReplacementList->AddItem(new BStringItem(((Synonym*)
					fSynonymList->ItemAt(i))->value.String()));
			}
			
			fReplacementList->Select(0);
			
			fReplaceButton->SetEnabled(true);
			fCancelButton->SetEnabled(true);
			fSkipButton->SetEnabled(true);
			break;
		}
	}
	
	fScanner->SelectCurrentWord();
	if (IsActive())
		if (fScanner->HighlightCurrentWord(true) != B_OK)
			StopScanning();
}

void 
ThesaurusWindow::StopScanning()
{
	fDragger->Enable();
	if (fScanner) {
		delete fScanner;
		fScanner = 0;
	}
	
	fReplaceButton->SetEnabled(false);
	fCancelButton->SetEnabled(false);
	fSkipButton->SetEnabled(false);
	fCurrentWord->SetText("");

	// Clear out list
	for (int32 itemNum = fReplacementList->CountItems() - 1; itemNum >= 0;
		itemNum--)
		delete fReplacementList->RemoveItem(itemNum);
}


bool 
ThesaurusWindow::QuitRequested()
{
	if (IsActive() && fScanner)
		fScanner->HighlightCurrentWord(false);

	StopScanning();
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}