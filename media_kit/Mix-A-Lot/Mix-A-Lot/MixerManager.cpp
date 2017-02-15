/*
	
	MixerManager.cpp
	
	Handles the private Mix-A-Lot mixer.
	
*/
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <Entry.h>
#include <Path.h>
#include <MediaRoster.h>
#include <MediaAddOn.h>
#include <TimeSource.h>
#include <Debug.h>
#include <ParameterWeb.h>

#include "Channel.h"
#include "MixerManager.h"

const uint32 PARAMETER_GROUP_ARCHIVE = 'pGRP';
const uint32 PARAMETER_ARCHIVE = 'pARC';

MixerManager::MixerManager()
{
	m_mixerNode = 0;
	m_mixerOut = 0;
	m_systemIn = 0;
	memset(m_channels, 0, MIXER_MAX_CHANNELS*sizeof(Channel*));
	memset(m_params, 0, MIXER_MAX_CHANNELS*sizeof(BMessage*));
	
	if (CreateMixer() != B_OK)
		ClearMixer();
}

MixerManager::~MixerManager()
{
	KillMixer();
}

media_node* MixerManager::Mixer() const
{
	return m_mixerNode;
}

void MixerManager::SoundFileChanged(int32 index, const entry_ref* ref)
{
	// Lock access so that only one thing's disconnecting and re-
	// connecting at a time. This prevents race conditions on the
	// array entry in question.
	if (m_mixerLock.Lock()) {
		ASSERT(index >= 0 && index < MIXER_MAX_CHANNELS);
		Channel* ch = m_channels[index];

		// Save the old set of parameters for this channel;
		// we'll reapply them to the new channel if possible.
		// Since the mixer's parameter web is created
		// asynchronously, we have to defer the reapplication
		// of these parameters to the next notice we get
		// that the mixer's parameter web has changed.
		if (ch) {
			BMessage* params = new BMessage;
			if (ArchiveChannelParameters(index, params) == B_OK) {
				m_params[index] = params;
			} else {
				delete params;
			}
			delete ch;
			m_channels[index] = 0;
		}
		
		
		if (ref) {
			// Create a new Channel to deal with the new entry_ref,
			// and insert it into the same place in the array.
			const char* name;
			BEntry entry(ref);
			if (entry.InitCheck() == B_OK) {
				BPath path(&entry);
				if (path.InitCheck() == B_OK) {
					name = path.Leaf();
				} else {
					name = 0;
				}
			} else {
					name = 0;
			}
			ch = new Channel(name, *m_mixerNode, ref);
			m_channels[index] = ch;
		}
		m_mixerLock.Unlock();
	}
}

bool MixerManager::ChannelAvailable(int32 index)
{
	if (index < 0 || index >= MIXER_MAX_CHANNELS)
		return false;

	// channel is only available if there isn't a
	// unassigned channel at a lower index
	for (int32 i=0; i<index; i++) {
		Channel* ch = m_channels[i];
		if (! (ch && ch->GetRef())) {
			return false;
		}
	}
	
	return true;
}

void MixerManager::SetMixerControls()
{
	for (int32 i=0; i<MIXER_MAX_CHANNELS; i++) {
		ApplyChannelParameters(i);
	}
}

void MixerManager::ClearMixer()
{
	delete m_mixerNode;
	delete m_mixerOut;
	delete m_systemIn;

	m_mixerNode = 0;
	m_mixerOut = 0;
	m_systemIn = 0;
}

status_t MixerManager::CreateMixer()
{
	status_t err;

	// This function creates, connects, and starts our mixer node,
	// in that order. This order is the reverse order that you would
	// do things when you're killing the mixer (see below).
	
	// Step 1. Create our own mixer node.
	fprintf(stderr, "MixerManager::CreateMixer() - Looking for a dormant mixer node...\n");
	dormant_node_info mixer_dormant_info;
	int32 mixer_count = 1; // for now, we only care about the first one we find.
	err = BMediaRoster::Roster()->GetDormantNodes(&mixer_dormant_info, &mixer_count,
		0, 0, 0, B_SYSTEM_MIXER, 0);
	if (err != B_OK) {
		fprintf(stderr, "Can't find the mixer node: %s\n", strerror(err));
		return err;
	}
	
	fprintf(stderr, "MixerManager::CreateMixer() - Instantiating a mixer node...\n");
	m_mixerNode = new media_node;
	err = BMediaRoster::Roster()->InstantiateDormantNode(mixer_dormant_info, m_mixerNode);
	if (err != B_OK) {
		fprintf(stderr, "Can't instantiate the mixer node: %s\n", strerror(err));
		return err;
	}
	
	// Step 2. Connect our mixer to the system mixer.
	fprintf(stderr, "MixerManager::CreateMixer() - Looking for the system mixer...\n");
	media_node systemMixer;
	err = BMediaRoster::Roster()->GetAudioMixer(&systemMixer);
	if (err != B_OK) {
		fprintf(stderr, "Can't find the system mixer: %s\n", strerror(err));
		return err;
	}
	
	fprintf(stderr, "MixerManager::CreateMixer() - Getting the system mixer inputs...\n");
	m_systemIn = new media_input;
	int32 count;
	err = BMediaRoster::Roster()->GetFreeInputsFor(systemMixer, m_systemIn, 1, &count);
	if (err != B_OK) {
		fprintf(stderr, "Can't get a system mixer input: %s\n", strerror(err));
		return err;
	}
	
	fprintf(stderr, "MixerManager::CreateMixer() - Getting the instantiated mixer outputs...\n");
	m_mixerOut = new media_output;
	err = BMediaRoster::Roster()->GetFreeOutputsFor(*m_mixerNode, m_mixerOut, 1, &count);
	if (err != B_OK) {
		fprintf(stderr, "Can't get the mixer output: %s\n", strerror(err));
		return err;
	}
	
	fprintf(stderr, "MixerManager::CreateMixer() - Connecting the instantiated mixer to the system mixer...\n");
	media_format fmt(m_mixerOut->format);
	err = BMediaRoster::Roster()->Connect(m_mixerOut->source, m_systemIn->destination,
		&fmt, m_mixerOut, m_systemIn);
	if (err != B_OK) {
		fprintf(stderr, "Can't connect the mixer to the system mixer: %s\n", strerror(err));
		return err;
	}
	
	// Step 3. Set our mixer's time source to the preferred time source.
	fprintf(stderr, "MixerManager::CreateMixer() - Looking for the preferred time source...\n");
	media_node timeSource;
	err = BMediaRoster::Roster()->GetTimeSource(&timeSource);
	if (err != B_OK) {
		fprintf(stderr, "Can't get the preferred time source: %s\n", strerror(err));
		return err;
	}

	fprintf(stderr, "MixerManager::CreateMixer() - Setting the instantiated mixer's time source...\n");	
	err = BMediaRoster::Roster()->SetTimeSourceFor(m_mixerNode->node, timeSource.node);
	if (err != B_OK) {
		fprintf(stderr, "Can't set the time source for the mixer: %s\n", strerror(err));
		return err;
	}

	// Step 4. Start our mixer's time source if need be. Chances are, it
	// won't need to be, but if we forget to do this, our mixer might not
	// do anything at all.
	fprintf(stderr, "MixerManager::CreateMixer() - Cloning the instantiated mixer's time source...\n");
	BTimeSource* mixerTimeSource = BMediaRoster::Roster()->MakeTimeSourceFor(*m_mixerNode);
	if (! mixerTimeSource) {
		fprintf(stderr, "Can't get the mixer's time source!\n");
		return B_ERROR;
	}
	
	if (!mixerTimeSource->IsRunning()) {
		fprintf(stderr, "MixerManager::CreateMixer() - Starting the instantiated mixer's time source...\n");
		status_t err = BMediaRoster::Roster()->StartNode(mixerTimeSource->Node(),
			BTimeSource::RealTime());
		if (err != B_OK) {
			fprintf(stderr, "Can't start the time source: %s\n", strerror(err));
			mixerTimeSource->Release();
			return err;
		}
	}

	// Step 5. Start up the mixer. 
	fprintf(stderr, "MixerManager::CreateMixer() - Starting the instantiated mixer...\n");
	bigtime_t tpNow = mixerTimeSource->Now();
	err = BMediaRoster::Roster()->StartNode(*m_mixerNode, tpNow + 10000);
	if (err != B_OK) {
		fprintf(stderr, "Can't start the mixer: %s\n", strerror(err));
		mixerTimeSource->Release();
		return err;
	}
	mixerTimeSource->Release();

	fprintf(stderr, "MixerManager::CreateMixer() - The mixer is ready to rumble!\n");		
	return B_OK;			
}

void MixerManager::KillMixer()
{
	if (! m_mixerNode)
		return;

	// Step 1. Disconnect and delete the inputs. It's VERY IMPORTANT
	// to stop, disconnect, and release the upstream nodes BEFORE the
	// downstream nodes, else a buffer-recycling deadlock is quite
	// likely. 
	printf("MixerManager::KillMixer() - Destroying the inputs...\n");		
	int32 i = MIXER_MAX_CHANNELS;
	while (--i >= 0) {
		delete m_channels[i];
		m_channels[i] = 0;
	}

	// Step 2. Stop, disconnect, and release the mixer node, in that
	// order. Not all nodes will robustly handle situations where you
	// mix the order of these calls, so it's good to get into the habit
	// of doing it this way.
	status_t err;
	printf("MixerManager::KillMixer() - Stopping the mixer...\n");		
	err = BMediaRoster::Roster()->StopNode(*m_mixerNode, 0, true);
	if (err != B_OK) {
		fprintf(stderr, "Can't stop the mixer: %s\n", strerror(err));
	}

	ASSERT(m_mixerOut && m_systemIn);
		
	printf("MixerManager::KillMixer() - Disconnecting the mixer...\n");		
	err = BMediaRoster::Roster()->Disconnect(m_mixerOut->node.node, m_mixerOut->source,
		m_systemIn->node.node, m_systemIn->destination);
	if (err != B_OK) {
		fprintf(stderr, "Can't disconnect the mixer: %s\n", strerror(err));
	}
	
	printf("MixerManager::KillMixer() - Deleting the mixer...\n");		
	err = BMediaRoster::Roster()->ReleaseNode(*m_mixerNode);
	if (err != B_OK) {
		fprintf(stderr, "Can't delete the mixer: %s\n", strerror(err));
	}
	
	ClearMixer();
	printf("MixerManager::KillMixer() - The mixer is dead.\n");
}

status_t MixerManager::ArchiveChannelParameters(int32 index, BMessage* message)
{
	if (! m_mixerNode)
		return B_ERROR;
		
	if (index < 0 || index >= MIXER_MAX_CHANNELS)
		return B_BAD_INDEX;
			
	status_t err;
	
	// Find the parameter group associated with this channel
	BParameterWeb* web;
	err = BMediaRoster::Roster()->GetParameterWebFor(*m_mixerNode, &web);
	if (err != B_OK)
		return err;
		
	BParameterGroup* group = FindChannelGroup(web, index);
	if (group) {
		err =  ArchiveChannelParameters(group, message);
	} else {
		err = B_ERROR;
	}
	delete web;
	return err;
}

status_t MixerManager::ArchiveChannelParameters(BParameterGroup* group, BMessage* message)
{
	message->what = PARAMETER_GROUP_ARCHIVE;
	int32 len = group->CountParameters();
	for (int32 i=0; i<len; i++) {
		BParameter* p = group->ParameterAt(i);
		ArchiveParameter(p, message);
	}
	return B_OK;
}

status_t MixerManager::ArchiveParameter(BParameter* p, BMessage* message)
{
	status_t err;
	BMessage pmsg(PARAMETER_ARCHIVE);
	fprintf(stderr, "Handling parameter: %s %s\n", p->Name(), p->Kind());
	err = pmsg.AddInt32("ID", p->ID());
	if (err != B_OK)
		return err;
		
	err = pmsg.AddString("Name", p->Name());
	if (err != B_OK)
		return err;
		
	err = pmsg.AddString("Kind", p->Kind());
	if (err != B_OK)
		return err;
		
	err = pmsg.AddInt32("Type", (int32) p->Type());
	if (err != B_OK)
		return err;
		
	err = pmsg.AddInt32("Media Type", (int32) p->MediaType());
	if (err != B_OK)
		return err;
		
	err = pmsg.AddString("Unit", p->Unit());
	if (err != B_OK)
		return err;
		
	int32 len = p->CountInputs();
	for (int32 i=0; i<len; i++) {
		err = pmsg.AddInt32("Inputs", p->InputAt(i)->ID());
		if (err != B_OK)
			return err;
	}
	len = p->CountOutputs();
	for (int32 i=0; i<len; i++) {
		err = pmsg.AddInt32("Outputs", p->OutputAt(i)->ID());
		if (err != B_OK)
			return err;
	}

	int32 ch = p->CountChannels();	
	err = pmsg.AddInt32("Channels", ch);
	if (err != B_OK)
		return err;

	if (p->Type() != BParameter::B_NULL_PARAMETER) {
		type_code t = p->ValueType();
		err = pmsg.AddInt32("Value Type", (int32) t);
		if (err != B_OK)
			return err;
			
		size_t valSize;
		switch (t) {
		case B_INT32_TYPE:
			valSize = sizeof(int32);
			break;
		case B_FLOAT_TYPE:
			valSize = sizeof(float);
			break;
		default:
			fprintf(stderr, "MixerManager::ArchiveParameter: unknown value type!\n");
			return B_BAD_TYPE;
		}
		size_t dataSize = ch*valSize;
		uint8* data = new uint8[dataSize];
		bigtime_t change;
		p->GetValue(data, &dataSize, &change);
		
		if (! strcmp(p->Kind(), B_GAIN)) {
			if (ch == 1) {
				fprintf(stderr, "Mono: gain = %g\n", *((float*)data));
			} else {
				fprintf(stderr, "Stereo: gain = [%g, %g]\n", *((float*)data), *(((float*)data)+1));
			}
		} else if (! strcmp(p->Kind(), B_MUTE)) {
			fprintf(stderr, "ID = %#010lx, mute = %#010lx\n", p->ID(), *((int32*)data));
		}
		
		err = pmsg.AddData("Value", B_RAW_TYPE, data, dataSize);
		if (err != B_OK)
			return err;
			
		err = pmsg.AddInt64("Value Changed", (int64) change);
		if (err != B_OK)
			return err;				
	}
		 				
	err = message->AddMessage("Parameters", &pmsg);
	return err;
}

BParameterGroup* MixerManager::FindChannelGroup(BParameterWeb* web, int32 index)
{
	BParameterGroup* group = web->GroupAt(0);
	group = group->GroupAt(1);
	if (group && (! strcmp(group->Name(), "Channels"))) {
		group = group->GroupAt(index);
		if (group && (atoi(group->Name()) == index+1)) {
			return group;
		}
	}
	return 0;
}

status_t MixerManager::ApplyChannelParameters(int32 index)
{
	if (! m_mixerNode)
		return B_ERROR;
		
	BMessage* msg = m_params[index];
	if (! msg) {
		return B_OK;
	}
	
	m_params[index] = 0;

	status_t err;

	// Find the parameter group associated with this channel
	BParameterWeb* web;
	err = BMediaRoster::Roster()->GetParameterWebFor(*m_mixerNode, &web);
	if (err == B_OK) {	
		BParameterGroup* group = FindChannelGroup(web, index);
		if (group) {
			err =  ApplyChannelParameters(group, msg);
		} else {
			err = B_ERROR;
		}
	}
	
	delete web;
	delete msg;
	return err;
}

status_t MixerManager::ApplyChannelParameters(BParameterGroup* group, const BMessage* message)
{
	BMessage pmsg;
	int32 i=0;
	while (message->FindMessage("Parameters", i++, &pmsg) == B_OK) {
		const char* name;
		status_t err;
		
		// Find the parameter we're trying to set
		err = pmsg.FindString("Name", &name);
		if (err != B_OK)
			continue;
		
		BParameter* p = 0;
		bool found = false;
		int32 len = group->CountParameters();
		for (int32 i=0; i<len; i++) {	
			p = group->ParameterAt(i);
			if (! strcmp(p->Name(), name)) {
				found = true;
				break;
			}
		}
		
		if (! found)
			continue;
		
		// Make sure the parameter has a settable value
		BParameter::media_parameter_type t = p->Type();
		if (t != BParameter::B_DISCRETE_PARAMETER && t != BParameter::B_CONTINUOUS_PARAMETER)
			continue;
		
		// Handle gain separately, so that we convert correctly between mono/stereo
		if (! strcmp(p->Kind(), B_GAIN)) {
			HandleGain(group, p, &pmsg, message);
			continue;
		}
		
		// Find the archived value
		ssize_t dataSize; 
		const uint8* data;
		err = pmsg.FindData("Value", B_RAW_TYPE, (const void**) &data, &dataSize);
		if (err != B_OK)
			continue;
		
		// Set the archived value
		p->SetValue(data, dataSize, 0);
	}
	return B_OK;
}

void MixerManager::HandleGain(BParameterGroup* group, BParameter* p,
	const BMessage* gainMessage, const BMessage* message)
{
	status_t err;
	int32 oldCh;
	err = gainMessage->FindInt32("Channels", &oldCh);
	if (err != B_OK)
		return;

	// Find the archived value
	ssize_t dataSize; 
	const float* data;
	err = gainMessage->FindData("Value", B_RAW_TYPE, (const void**) &data, &dataSize);
	if (err != B_OK)
		return;
		
	int32 newCh = p->CountChannels();
	if (oldCh == newCh) {
		if (oldCh == 1) {
			fprintf(stderr, "Mono to mono: gain = %g\n", *data);
		} else {
			fprintf(stderr, "Stereo to stereo: gain = [%g, %g]\n", data[0], data[1]);
		}
		p->SetValue(data, dataSize, 0);
	} else if (oldCh == 1) {
		// convert mono to stereo
		BMessage panMessage;
		int32 i=0;
		bool found = false;
		while (message->FindMessage("Parameters", i++, &panMessage) == B_OK) {
			const char* name = panMessage.FindString("Kind");
			if (name && (! strcmp(name, B_BALANCE))) {
				found = true;
				break;
			}
		}
		if (! found)
			return;
			
		ssize_t panSize;
		const float* panData;
		err = panMessage.FindData("Value", B_RAW_TYPE, (const void**) &panData, &panSize);
		if (err != B_OK)
			return;
		
		float oldData = (data[0] + 60) / 78;
		float newData[2];
		if (*panData < 0) {
			newData[0] = oldData;
			newData[1] = oldData * (1 + *panData);
		} else {
			newData[0] = oldData * (1 - *panData);
			newData[1] = oldData;
		}
		newData[0] = newData[0] * 78 - 60;
		newData[1] = newData[1] * 78 - 60;
		
		fprintf(stderr, "Mono to stereo: G=%g, P=%g -> G=[%g, %g]\n", oldData, *panData,
			newData[0], newData[1]);
			
		p->SetValue(newData, 2*sizeof(float), 0);
	} else {
		// stereo to mono
		BParameter* panParam = 0;
		bool found = false;
		int32 len = group->CountParameters();
		for (int32 i=0; i<len; i++) {
			panParam = group->ParameterAt(i);
			if (! strcmp(panParam->Kind(), B_BALANCE)) {
				found = true;
				break;
			}
		}
		if (! found)
			return;
		
		float newGain;
		float newPan;
		float oldData[2];
		oldData[0] = (data[0] + 60) / 78;
		oldData[1] = (data[1] + 60) / 78;
		newGain = (oldData[0] >= oldData[1]) ? oldData[0] : oldData[1];
		newPan = (oldData[1] - oldData[0]) / newGain;
		newGain = newGain * 78 - 60;
		fprintf(stderr, "Stereo to mono: G=[%g, %g] => G=%g, P=%g\n", oldData[0], oldData[1],
			newGain, newPan);
		p->SetValue(&newGain, sizeof(float), 0);
		panParam->SetValue(&newPan, sizeof(float), 0);
	}
}
