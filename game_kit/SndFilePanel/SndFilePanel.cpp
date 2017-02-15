// SndFilePanel.cpp
// ------------
// A BFilePanel derivative that lets you select and preview
// audio files.
//
// Copyright 2000, Be Incorporated.   All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#include "SndFilePanel.h"

#include <File.h>
#include <Messenger.h>
#include <NodeInfo.h>
#include <String.h>
#include <Window.h>

#include <stdio.h>

const uint32 kPlayStop = 'plst'; // message the Play/Stop button sends
const uint32 kUpdateBtn = 'btup'; // should we update the button title?

SndFilePanel::SndFilePanel(BHandler* handler)
	: BFilePanel(B_OPEN_PANEL,new BMessenger(handler),NULL,
			B_FILE_NODE,false,NULL,new SndFileFilter(),false,true),
		mSndFile(NULL), mPlayBtn(NULL), mBtnUpdater(NULL)
{
	BView* view;

	if (Window()->Lock()) {
		// add this object to the window's looper's list of handlers
		// (we inherit from BHandler, too, remember?)
		Window()->AddHandler(this);
		
		// get ahold of the Cancel button so we can position
		// and size the Play/Stop button relative to it
		BView* cancel = Window()->FindView("cancel button");
		if (cancel == NULL) {
			// that's not right...
			goto exit;
		}

		// add the Play/Stop button
		view = Window()->ChildAt(0); // get the background view
		if (view != NULL) {
			// Frame() gets the coordinates in the parent view
			BRect r(cancel->Frame());
			// move it left 10 pels less than the distance between
			// Cancel button and the left edge of the panel
			r.OffsetBy(-(r.left - view->Bounds().left - 10),0);
			// make sure to B_FOLLOW_BOTTOM so resizing works right!
			mPlayBtn = new BButton(r,"PlayStop","Play",
				new BMessage(kPlayStop),B_FOLLOW_LEFT|B_FOLLOW_BOTTOM);
			// it's essential to set the button's target to this,
			// otherwise our MessageReceived() won't get the button's
			// messages
			mPlayBtn->SetTarget(this);
			mPlayBtn->SetEnabled(false);
			view->AddChild(mPlayBtn);
		}

exit:
		Window()->Unlock();
	}

	SetButtonLabel(B_DEFAULT_BUTTON,"Select");
	Window()->SetTitle("Select A Sound File");
}

SndFilePanel::~SndFilePanel(void)
{
	delete RefFilter();
	// deleting BFileGameSound stops it too
	delete mSndFile;
	delete mBtnUpdater;
}

void SndFilePanel::SelectionChanged(void)
{
	status_t err;
	entry_ref ref;
	
	if (mSndFile) {
		delete mSndFile;
		mSndFile = NULL;
		mPlayBtn->SetEnabled(false);
	}
	// Rewind() is essential to make sure GetNextSelectedRef()
	// gets the first in the list of selected refs --
	// even if there's only one selected!
	Rewind();
	err = GetNextSelectedRef(&ref);
	if (err == B_OK) {
		BNode node(&ref);
		if (!node.IsDirectory()) {
			delete mSndFile;
			mSndFile = new BFileGameSound(&ref,false);
			if (mSndFile->InitCheck() == B_OK) {
				mPlayBtn->SetEnabled(true);
			}
		}
	}
}

void SndFilePanel::WasHidden(void)
{
	// This will be called any time the user causes
	// the panel to be hidden, but not if some
	// SndFilePanel member function calls Hide().
	// BMessageRunner will be restarted the next time
	// Play button is clicked (see MessageReceived(), below.

	// kill the BMessageRunner
	delete mBtnUpdater;
	// set it to NULL as lazy way out of double-delete trouble
	mBtnUpdater = NULL;
}

void SndFilePanel::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case kPlayStop:
		if (mSndFile != NULL) {
			if (mSndFile->IsPlaying()) {
				mSndFile->StopPlaying();
				mPlayBtn->SetLabel("Play");
			}
			else {
				mSndFile->StartPlaying();
				mPlayBtn->SetLabel("Stop");
				
				// if necessary, start the BMessageRunner
				// that will will periodically check to see if
				// the button label needs to be updated
				if (mBtnUpdater == NULL) {
					mBtnUpdater = new BMessageRunner(
						BMessenger(this),
						new BMessage(kUpdateBtn),
						500000/* every .5 sec */);
				}
			}
		}
		break;
	case kUpdateBtn:
		//fprintf(stderr,"got kUpdateBtn\n");
		if (mSndFile != NULL) {
			if (!mSndFile->IsPlaying()) {
				mPlayBtn->SetLabel("Play");
			}
		}
		break;
	}
}

SndFileFilter::SndFileFilter(void)
	: BRefFilter()
{
}

SndFileFilter::~SndFileFilter(void)
{
}

bool SndFileFilter::Filter(const entry_ref* ref,BNode* node,
	struct stat* st,const char* filetype)
{
	bool admitIt = false;
	char type[256];
	const BString mask("audio");
	BNodeInfo nodeInfo(node);

	if (node->IsDirectory()) {
		admitIt = true;
	}
	else {
		nodeInfo.GetType(type);
		// allow all files with supertype "audio"
		admitIt = (mask.Compare(type,mask.CountChars()) == 0);
	}

	return (admitIt);
}
