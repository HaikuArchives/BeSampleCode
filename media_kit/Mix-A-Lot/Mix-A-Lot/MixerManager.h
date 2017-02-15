/*
	
	MixerManager.cpp
	
*/
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _MixerManager_h
#define _MixerManager_h

#include <SupportDefs.h>
#include <Locker.h>

struct media_node;
struct media_output;
struct media_input;

#define MIXER_MAX_CHANNELS 4

class Channel;

class MixerManager
{
public:
	MixerManager();
	~MixerManager();
		
	media_node* Mixer() const;
	
	void SoundFileChanged(int32 index, const entry_ref* ref);
	bool ChannelAvailable(int32 index);
	void SetMixerControls();
	
	status_t ArchiveChannelParameters(int32 index, BMessage* message);
	status_t ArchiveChannelParameters(BParameterGroup* group, BMessage* message);
	status_t ArchiveParameter(BParameter*p, BMessage* message);
	BParameterGroup* FindChannelGroup(BParameterWeb* web, int32 index);
	status_t ApplyChannelParameters(int32 index);
	status_t ApplyChannelParameters(BParameterGroup* group, const BMessage* message);
	void HandleGain(BParameterGroup* group, BParameter* p,
		const BMessage* gainMessage, const BMessage* message);
	
private:
	status_t CreateMixer();
	void ClearMixer();
	void KillMixer();

	media_node* m_mixerNode;
	media_output* m_mixerOut;
	media_input* m_systemIn;
	Channel* m_channels[MIXER_MAX_CHANNELS];
	BLocker m_mixerLock;
	BMessage* m_params[MIXER_MAX_CHANNELS];
};

#endif /* _MixerManager_h */
