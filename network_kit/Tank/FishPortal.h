/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef FishPortal_H
#define FishPortal_H

#include <Messenger.h>
#include <Handler.h>
#include <MessageRunner.h>
#include <List.h>
#include <socket.h>
#include "dfp.h"

const uint32 SEND_FISH = 'fIsH';
const uint32 NET_FISH = 'hSiF';

const uint32 TANK_STATUS = 'tAnK';
const uint32 PORTAL_INIT = 'goPO';

const uint32 DFP_PING_MESSAGE = 'fPIN';
const uint32 DFP_PONG_MESSAGE = 'fPON';
const uint32 DFP_SYNC_MESSAGE = 'fSYN';
const uint32 DFP_SEND_MESSAGE = 'fSEN';
const uint32 DFP_NACK_MESSAGE = 'fNAK';
const uint32 DFP_ACK_MESSAGE  = 'fACK';
const uint32 PING_PULSE = 'fPUL';

const uint32 DFP_LOCATE_TYPE = 'Floc';
const uint32 DFP_CONFIRM_TYPE = 'Fcon';
const uint32 DFP_TRANSFER_TYPE = 'Ftra';

class Fish
{
public:
	Fish(uint8 x, uint8 y, int8 dx, int8 dy, char *name,
		uint8 creator_x, uint8 creator_y, uint16 ttl);
	Fish(dfp_pkt_transfer *dfp);
	void Wrap(dfp_pkt_transfer *dfp);
	bool operator==(const Fish &other) const;

	uint8	x, y;
	int8	dx, dy;
	uint8	creator_x;
	uint8	creator_y;
	uint32	timestamp;
	char 	name[16];
	uint16	ttl;
};


class FishPortal : public BHandler {
public:
	FishPortal(BMessenger target);
	~FishPortal();
	virtual	void	MessageReceived(BMessage *message);
	
private:
	void PingTank(uint8 x, uint8 y);
	void SendFish(const dfp_pkt_transfer *dfp);

	static int32	net_thread_stub(void *arg);
	int32			net_thread(void);

	uint8			tank_x, tank_y;
	bool			tank_left, tank_right, tank_up, tank_down;

	BList			fishList;
	
	BMessageRunner	*pulseRunner;
	BMessenger		Courier;
	
	struct 			sockaddr_in dst;
	int 			dstSocket;

	thread_id		net_thread_id;
	int				net_thread_socket;
};

#endif