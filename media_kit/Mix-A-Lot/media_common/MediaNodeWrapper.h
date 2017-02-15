/*******************************************************************************
/
/	File:			MediaNodeWrapper.h
/
/   Description:	A application-based object-oriented API for
/					working with media_nodes.
/
*******************************************************************************/
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _MediaNodeWrapper_h
#define _MediaNodeWrapper_h

#include <MediaNode.h>

class MediaNodeWrapper
{
public:
	// Constructors, destructors, operators
	MediaNodeWrapper();
	
	// locked = true means that you shouldn't be able to "do
	// anything" to the node; i.e. start/stop/slave
	// will have no effect (NOTE: but Release still will!).
	// Use this to wrap around system nodes, such as the audio
	// output, that you shouldn't be tampering with.
	MediaNodeWrapper(const media_node& node, bool locked = false);
	MediaNodeWrapper(const MediaNodeWrapper& node);
	~MediaNodeWrapper();
	
	MediaNodeWrapper& operator=(const MediaNodeWrapper& node);

	bool Locked() const { return m_locked; }
	void SetLocked(bool locked) { m_locked = locked; }
	
	void Clear() { m_init = false; }
	
	// Media Node stuff	
	void Start(bigtime_t tpStart) const;
	void Stop(bigtime_t tpStop, bool synchronous) const;
	void Seek(bigtime_t tmSeekTo, bigtime_t tpSeekAt) const;
	void Preroll() const;
	void Roll(bigtime_t tpStart, bigtime_t tpStop, bigtime_t tmSeek) const;
	void Sync(bigtime_t tpSyncTo, bigtime_t timeout = B_INFINITE_TIMEOUT) const;
	void SetRunMode(BMediaNode::run_mode mode, bigtime_t delay = 0) const;

	
	// SlaveTo sets this node's time source to whatever the
	// master node's time source is, NOT necessarily to the
	// master node itself!
	void SlaveTo(MediaNodeWrapper master) const;
	BTimeSource* MakeTimeSource() const;
	
	// GetFreeOutput/GetFreeInput simply return the first
	// available output/input from the node.
	status_t GetFreeOutput(media_output* output) const;
	status_t GetFreeInput(media_input* input) const;

	// GetLatency returns the TOTAL latency for the node.
	status_t GetLatency(bigtime_t* latency) const;
	
	void Release() const;
	
	// File Interface stuff
	status_t SetRef(const entry_ref* ref, bool create=false, bigtime_t* out_duration=0) const;
	
private:
	media_node m_node;
	bool m_locked;
	bool m_init;
};

#endif /* _MediaNodeWrapper_h */
