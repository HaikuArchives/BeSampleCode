/*
	
	Channel.cpp
	
	Represents a single sound source and output connection.
	
*/

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#include <stdio.h>
#include <string.h>
#include <MediaAddOn.h>
#include <MediaRoster.h>
#include <TimeSource.h>
#include <Debug.h>
#include <Entry.h>

#include "Channel.h"
#include "Connection.h"
#include "NodeLooper.h"

Channel::Channel(const char* name, const media_node& destNode, const entry_ref* ref)
	: m_destNode(destNode)
{
	m_ref = 0;
	m_connection = 0;
	m_looper = 0;
	m_duration = 0;
	m_initStatus = CreateNode(name, ref);
	if (m_initStatus == B_OK) {
		SetRef(ref);		
	} else {
		ClearNode();
	}
}

Channel::~Channel()
{
	StopNode();
	Disconnect();
	ClearNode();
	delete m_ref;
}

status_t Channel::InitCheck() const
{
	return m_initStatus;
}

void Channel::SetRef(const entry_ref* ref)
{
	if (m_connection) {
		StopNode();
		Disconnect();
	}
	
	delete m_ref;
	m_ref = 0;
	
	if (ref) {
		m_ref = new entry_ref(*ref);
		status_t err = m_srcNode.SetRef(m_ref, false, &m_duration);
		if (err == B_OK) {
			if (Connect() == B_OK)
				StartNode();			
		}
	}
}

status_t Channel::CreateNode(const char* name, const entry_ref* ref)
{
	status_t err;
	
	ASSERT(ref);
		
	printf("Channel::CreateNode() - Creating sound input...\n");
	dormant_node_info dni;
	err = BMediaRoster::Roster()->SniffRef(*ref, B_BUFFER_PRODUCER | B_FILE_INTERFACE, &dni);
	if (err != B_OK) {
		fprintf(stderr, "Can't find a node that'll read this file: %s\n", name);
		return err;
	}
	
	media_node node;
	err = BMediaRoster::Roster()->InstantiateDormantNode(dni, &node);
	if (err != B_OK) {
		fprintf(stderr, "Can't instantiate the dormant node for this file: %s\n", name);
		return err;
	}
		
	m_srcNode = node;
	
	printf("Channel::CreateNode() - Setting the source's time source...\n");
	m_srcNode.SlaveTo(m_destNode);

	// how about a reasonable default duration of 2 secs?	
	m_looper = new NodeLooper(m_srcNode, 0, 2000000);
	printf("Channel::CreateNode() - Sound input node all set!...\n");	
	return B_OK;
}

status_t Channel::Connect()
{
	if (m_connection) {
		fprintf(stderr, "Connection already made!\n");
		return B_OK;
	}
			
	m_connection = Connection::Make(m_srcNode, m_destNode); 
	if (! m_connection) {
		fprintf(stderr, "Connection failed.\n");
		return B_ERROR;
	}
	
	printf("Channel::Connect() - all clear for dispatch!\n");
	return B_OK;
}

status_t Channel::Disconnect()
{
	if (! m_connection)
		return B_ERROR;
	
	delete m_connection;
	m_connection = 0;
	return B_OK;
}

void Channel::ClearNode()
{
	if (m_looper) {
		delete m_looper;
	}
	
	printf("Channel: releasing node\n");
	m_srcNode.Release();
	m_srcNode.Clear();
}

void Channel::StartNode()
{
	if (m_looper) {
		printf("Channel::StartNode() - starting now (dur = %Ld)\n", m_duration);
		m_looper->SetDuration(m_duration);
		m_looper->Start();
	}
}

void Channel::StopNode()
{
	if (m_looper) {
		printf("Channel::StopNode() - stopping now.\n");
		m_looper->Stop();	
	}
}
