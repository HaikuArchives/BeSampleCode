/*******************************************************************************
/
/	File:			Connection.h
/
/   Description:	A simple structure that completely describes a connection
/                   between two Nodes.
/
*******************************************************************************/
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _Connection_h
#define _Connection_h

#include <MediaNode.h>

class MediaNodeWrapper;

struct Connection
{
public:
	media_node_id src_node;
	media_node_id dest_node;
	media_source src;
	media_destination dest;
	media_format format;
	char* src_name;
	char* dest_name;
	
protected:
	// This connects the source to the destination if possible.
	Connection(media_source mySrc, media_destination myDest,
		const media_format& myFormat);
public:
	// If the source and destination are connected, deleting a
	// Connection disconnects the two.
	~Connection();

	// Tells you whether both ends of the connection are hooked up to something.
	bool IsConnected() const;

	// This is the easiest way to create a connection! It simply takes the first
	// available output and input from the upstream and downstream nodes and
	// hooks 'em up.
	// The new connection is created and returned to you. Its src and dest will
	// be null if the connection fails.
	static Connection* Make(MediaNodeWrapper upstream, MediaNodeWrapper downstream);
};

#endif /* _Connection_h */
