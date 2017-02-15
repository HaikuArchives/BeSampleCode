/*
	
	MixWin.h
	
*/
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _MixWin_h
#define _MixWin_h

#include <Window.h>

class MixerManager;
class SoundDock;

class MixWin : public BWindow 
{
public:
	MixWin();
	virtual ~MixWin();
	virtual	bool QuitRequested();
	virtual void MessageReceived(BMessage* message);
	
private:
	status_t Init();
	
	status_t UpdateMixerView();
	void CreateSoundDocks();
	SoundDock* CreateSoundDock(BView* parent);
	void UpdateLayout();
	void CenterView(BView* v, BPoint where);
	
	void SoundFileChanged(BMessage* message);
	void MediaWebChanged(BMessage* message);

	bool ChannelAvailable(SoundDock* dock);
	
	MixerManager* m_mixerManager;
	BView* m_mixerView;
	BView* m_dockBkg;
	BList m_docks;
	
	friend class SoundDock;
};

#endif /* _MixWin_h */
