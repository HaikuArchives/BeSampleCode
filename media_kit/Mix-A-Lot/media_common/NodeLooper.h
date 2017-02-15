/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _NodeLooper_h
#define _NodeLooper_h

#include <Locker.h>
#include "MediaNodeWrapper.h"

class NodeLooper
{
public:
	NodeLooper(MediaNodeWrapper node, bigtime_t start, bigtime_t dur);
	~NodeLooper();
	
	bigtime_t Duration() const { return m_tmDuration; }
	void SetDuration(bigtime_t dur) { m_tmDuration = dur; }
	
	bigtime_t StartTime() const { return m_tmStart; }
	void SetStartTime(bigtime_t start) { m_tmStart = start; }
	
	void Start();
	void Stop();

private:	
	thread_id m_looper;
	port_id m_port;
		
	MediaNodeWrapper m_node;
	bigtime_t m_tmStart;
	bigtime_t m_tmDuration;	
	bool m_running;
};

#endif /* _NodeLooper_h */
