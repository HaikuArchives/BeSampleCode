/*******************************************************************************
/
/	File:			Connection.cpp
/
/   Description:	A simple structure that completely describes a connection
/                   between two Nodes.
/
*******************************************************************************/
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <MediaRoster.h>
#include "Connection.h"
#include "MediaNodeWrapper.h"

Connection::Connection(media_source mySrc, media_destination myDest,
	const media_format& myFormat)
	: format(myFormat)
{
	media_input in;
	media_output out;

	status_t err = BMediaRoster::Roster()->Connect(mySrc, myDest, &format, &out, &in);
	if (err != B_OK) {
		src = media_source::null; dest = media_destination::null;
		src_name = 0; dest_name = 0;
	} else {
		src = out.source; dest = in.destination;
		src_node = out.node.node; dest_node = in.node.node;
		src_name = strdup(out.name); dest_name = strdup(in.name);
	}
}

Connection::~Connection()
{
	if (IsConnected())
		BMediaRoster::Roster()->Disconnect(src_node, src, dest_node, dest);
	if (src_name) free(src_name);
	if (dest_name) free(dest_name);
}

bool Connection::IsConnected() const
{
	return (src != media_source::null && dest != media_destination::null);
}

Connection* Connection::Make(MediaNodeWrapper upstream, MediaNodeWrapper downstream)
{
	status_t err;
	media_input in;
	media_output out;
	
	err = upstream.GetFreeOutput(&out);
	if (err != B_OK) out.source = media_source::null;
	err = downstream.GetFreeInput(&in);
	if (err != B_OK) in.destination = media_destination::null;
	
	return new Connection(out.source, in.destination, out.format);
}
