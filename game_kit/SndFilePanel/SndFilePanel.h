// SndFilePanel.h
// ------------
// A BFilePanel derivative that lets you select and preview
// audio files.
//
// Copyright 2000, Be Incorporated.   All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#ifndef _SNDFILEPANEL_H_
#define _SNDFILEPANEL_H_

#include <Button.h>
#include <Handler.h>
#include <FileGameSound.h>
#include <FilePanel.h>
#include <MessageRunner.h>

class SndFilePanel : public BFilePanel, public BHandler {
public:
	SndFilePanel(BHandler* handler);
	virtual ~SndFilePanel(void);
	
	virtual void SelectionChanged(void);
	virtual void WasHidden(void);
	
	virtual void MessageReceived(BMessage* msg);
	
private:
	BFileGameSound* mSndFile;
	BButton* mPlayBtn;
	// use a BMessageRunner to periodically remind us
	// to check whether mSndFile has stopped playing so
	// we can reset mPlayBtn's label to "Play".
	BMessageRunner* mBtnUpdater;
};

class SndFileFilter : public BRefFilter {
public:
	SndFileFilter(void);
	virtual ~SndFileFilter(void);
	
	virtual bool Filter(const entry_ref* ref,BNode* node,
		struct stat* st,const char* filetype);
};

#endif // #ifndef _SNDFILEPANEL_H_