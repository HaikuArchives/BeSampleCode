/*
	
	Channel.h
	
*/

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _Channel_h
#define _Channel_h

#include "MediaNodeWrapper.h"

class entry_ref;
class Connection;
class NodeLooper;

class Channel
{
public:
	Channel(const char* name, const media_node& destNode,
		const entry_ref* ref=0);
	~Channel();
	
	const entry_ref* GetRef() const { return m_ref; }
	void SetRef(const entry_ref* ref);
	status_t InitCheck() const;
	
private:
	status_t Connect();
	status_t Disconnect();
	
	status_t CreateNode(const char* name, const entry_ref* ref);
	void ClearNode();
	void StartNode();
	void StopNode();
	
	MediaNodeWrapper m_srcNode;
	const entry_ref* m_ref;	
	status_t m_initStatus;
	MediaNodeWrapper m_destNode;
	Connection* m_connection;
	NodeLooper* m_looper;
	bigtime_t m_duration;
};

#endif /* _Channel_h */