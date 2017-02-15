/*******************************************************************************
/
/	File:			MediaNodeWrapper.cpp
/
/   Description:	A application-based object-oriented API for
/					working with media_nodes.
/
*******************************************************************************/
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Debug.h>
#include <MediaRoster.h>
#include <TimeSource.h>
#include "MediaNodeWrapper.h"

MediaNodeWrapper::MediaNodeWrapper()
	: m_init(false)
{
}

MediaNodeWrapper::MediaNodeWrapper(const media_node& node, bool locked)
	: m_node(node), m_locked(locked), m_init(true)
{
}

MediaNodeWrapper::MediaNodeWrapper(const MediaNodeWrapper& node)
	: m_node(node.m_node), m_locked(node.m_locked), m_init(node.m_init)
{
}

MediaNodeWrapper::~MediaNodeWrapper()
{
}

MediaNodeWrapper& MediaNodeWrapper::operator=(const MediaNodeWrapper& node)
{
	if (&node != this) {
		m_node = node.m_node;
		m_locked = node.m_locked;
		m_init = node.m_init;
	}
	return *this;
}

void MediaNodeWrapper::Start(bigtime_t tpStart) const
{
	if (m_init && (! m_locked))
		BMediaRoster::Roster()->StartNode(m_node, tpStart);
}

void MediaNodeWrapper::Stop(bigtime_t tpStop, bool synchronous) const
{
	if (m_init && (! m_locked))
		BMediaRoster::Roster()->StopNode(m_node, tpStop, synchronous);
}

void MediaNodeWrapper::Seek(bigtime_t tmSeekTo, bigtime_t tpSeekAt) const
{
	if (m_init && (! m_locked))
		BMediaRoster::Roster()->SeekNode(m_node, tmSeekTo, tpSeekAt);
}

void MediaNodeWrapper::Preroll() const
{
	if (m_init && (! m_locked))
		BMediaRoster::Roster()->PrerollNode(m_node);
}

void MediaNodeWrapper::Roll(bigtime_t tpStart, bigtime_t tpStop,
	bigtime_t tmSeek) const
{
	if (m_init && (! m_locked))
		BMediaRoster::Roster()->RollNode(m_node, tpStart, tpStop, tmSeek);
}

void MediaNodeWrapper::Sync(bigtime_t tpSyncTo, bigtime_t timeout) const
{
	if (m_init)
		BMediaRoster::Roster()->SyncToNode(m_node, tpSyncTo, timeout);
}

void MediaNodeWrapper::SlaveTo(MediaNodeWrapper master) const
{
	if (m_init && (! m_locked)) {
		BTimeSource* ts = master.MakeTimeSource();
		if (ts) {
			BMediaRoster::Roster()->SetTimeSourceFor(m_node.node, ts->Node().node);
			ts->Release();
		}
	}
}

BTimeSource* MediaNodeWrapper::MakeTimeSource() const
{
	if (m_init)
		return BMediaRoster::Roster()->MakeTimeSourceFor(m_node);
	else return 0;
}

status_t MediaNodeWrapper::GetFreeOutput(media_output* output) const
{
	if (m_init) {
		int32 count;
		return BMediaRoster::Roster()->GetFreeOutputsFor(m_node, output, 1, &count);
	} else return B_ERROR;
}

status_t MediaNodeWrapper::GetFreeInput(media_input* input) const
{
	if (m_init) {
		int32 count;
		return BMediaRoster::Roster()->GetFreeInputsFor(m_node, input, 1, &count);
	} else return B_ERROR;
}

status_t MediaNodeWrapper::GetLatency(bigtime_t* latency) const
{
	if (m_init)
		return BMediaRoster::Roster()->GetLatencyFor(m_node, latency);
	else return B_ERROR;
}

void MediaNodeWrapper::Release() const
{
	if (m_init)
		BMediaRoster::Roster()->ReleaseNode(m_node);
}

void MediaNodeWrapper::SetRunMode(BMediaNode::run_mode mode, bigtime_t delay) const
{
	if (m_init && (! m_locked))
		BMediaRoster::Roster()->SetProducerRunModeDelay(m_node, delay, mode);
}

status_t MediaNodeWrapper::SetRef(const entry_ref* ref, bool create,
	bigtime_t* out_duration) const
{
	bigtime_t dummyDur;
	bigtime_t* dur = (out_duration) ? out_duration : &dummyDur;
	
	if (! m_init)
		return B_NO_INIT;
		
	if (m_locked)
		return B_NOT_ALLOWED;
		
	return BMediaRoster::Roster()->SetRefFor(m_node, *ref, create, dur);
}
