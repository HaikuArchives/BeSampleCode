/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#ifndef _NET_BUF_LIST_H
#define _NET_BUF_LIST_H

#include <BeBuild.h>
#include <NetPacket.h>

typedef struct _netbuflist {
	BNetPacket *buf;
	struct _netbuflist *next;
} *_netbuflist_t;

class _NetBufList {
	_netbuflist_t list;
	_netbuflist_t freelist;
public:
	_NetBufList(void);
	~_NetBufList(void);
	void ChainToEnd(BNetPacket *packet);
	BNetPacket *RemoveHead();
};

#endif /* _NET_BUF_LIST_H */


