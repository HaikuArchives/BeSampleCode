/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "FishPortal.h"
#include <stdio.h>
#include <stdlib.h>
#include <socket.h>
#include <string.h>
#include <OS.h>
#include "dfp.h"

Fish::Fish(uint8 Nx, uint8 Ny, int8 Ndx, int8 Ndy, char *Nname, uint8 Ncreator_x, uint8 Ncreator_y, uint16 Nttl)
{
	x=Nx;
	y=Ny;
	dx=Ndx;
	dy=Ndy;
	creator_x =Ncreator_x;
	creator_y =Ncreator_y;
	timestamp = random();
	ttl = Nttl;

	if (Nname) {
		memcpy(name, Nname, 16 * sizeof(char) );
	} else {
		memcpy(name, "NewFish        \0", 15 * sizeof(char) );
	}
}

/*********************/

Fish::Fish(dfp_pkt_transfer *dfp)
{
	x = dfp->fish.x;
	y = dfp->fish.y;
	dx = dfp->fish.dx;
	dy = dfp->fish.dy;
	creator_x = dfp->uid.creator_tank_x;
	creator_y = dfp->uid.creator_tank_y;
	ttl = dfp->fish.ttl;
	memcpy(name, dfp->fish.name, 16);
	memcpy(&timestamp, dfp->uid.timestamp, 4);
}

/*********************/

void
Fish::Wrap(dfp_pkt_transfer *dfp)
{
	dfp->fish.x = x;
	dfp->fish.y = y;
	dfp->fish.dx = dx;
	dfp->fish.dy = dy;
	dfp->uid.creator_tank_x = creator_x;
	dfp->uid.creator_tank_y = creator_y;
	dfp->fish.ttl = ttl;
	memcpy(dfp->fish.name, name, 16);
	memcpy(dfp->uid.timestamp, &timestamp, 4);
}

/*********************/

/* two fish are equal if their uid fields match */
bool 
Fish::operator==(const Fish &other) const
{
	if (this->creator_x != other.creator_x) return(false);
	if (this->creator_y != other.creator_y) return(false);
	if (this->timestamp != other.timestamp) return(false);
	return(true);
}

/*****************/
/*****************/

FishPortal::FishPortal(BMessenger target) : BHandler("FishPortal"), Courier(target)
{
	srand(real_time_clock());

	net_thread_socket=-1;
	net_thread_id = spawn_thread(net_thread_stub, "DFP net_thread", B_NORMAL_PRIORITY, this);
	resume_thread(net_thread_id);

	dst.sin_family = AF_INET;
	dst.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	dst.sin_port = htons(DEFAULT_DFP_PORT);
			
    if((dstSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        perror("socket");
		    
}

/*****************/

FishPortal::~FishPortal()
{
	kill_thread(net_thread_id);
	if(net_thread_socket >= 0) closesocket(net_thread_socket);
	if(dstSocket >= 0) closesocket(dstSocket);
	delete(pulseRunner);
}

/*****************/

void
FishPortal::PingTank(uint8 tx, uint8 ty)
{
	dfp_pkt_transfer dfp;
    dfp.base.src_tank_x = tank_x;
    dfp.base.src_tank_y = tank_y;
    dfp.base.dst_tank_x = tx;
    dfp.base.dst_tank_y = ty;
    dfp.base.size = htons(DFP_SIZE_LOCATE);
    dfp.base.type = DFP_PKT_PING;
    dfp.base.pad = 0;
    dfp.base.version = htons(DFP_VERSION);

	sendto(dstSocket, (void *) &dfp, sizeof(dfp), 0, (sockaddr *) &dst, sizeof(dst) );
}

/*****************/

void
FishPortal::SendFish(const dfp_pkt_transfer *dfp)
{
	dfp_pkt_transfer sdfp;
	memcpy(&sdfp, dfp, sizeof(*dfp));

    sdfp.base.src_tank_x = tank_x;
    sdfp.base.src_tank_y = tank_y;
    sdfp.base.size = htons(DFP_SIZE_LOCATE);
    sdfp.base.type = DFP_PKT_SEND_FISH;
    sdfp.base.pad = 0;
    sdfp.base.version = htons(DFP_VERSION);

	sendto(dstSocket, (void *) &sdfp, sizeof(sdfp), 0, (sockaddr *) &dst, sizeof(dst) );
}

/*****************/

int32 
FishPortal::net_thread_stub(void *arg)
{
	FishPortal *obj = (FishPortal *)arg;
	obj->net_thread();
	return(0);
}

/*****************/

int32 
FishPortal::net_thread(void)
{
    struct sockaddr_in src;

    src.sin_family = AF_INET;
    src.sin_addr.s_addr = htonl(INADDR_ANY);
    src.sin_port = htons(DEFAULT_DFP_PORT);
    
    if((net_thread_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        perror("socket");
    
    if(bind(net_thread_socket, (struct sockaddr *) &src, sizeof(src)) == -1)
        perror("net_thread: bind");

	dfp_pkt_transfer	dfp;
    for(;;){
        if(recvfrom(net_thread_socket,(void *)&dfp,sizeof(dfp),0,NULL,NULL) < 0) {
            perror("recvfrom");
        }

		/* might as well swap the base data here */
		dfp.base.version = ntohs(dfp.base.version);
		dfp.base.size = ntohs(dfp.base.size);
		
		switch(dfp.base.type) {
		/* PING and PONG carry no extra data--just knowing that */
		/* the other tank exists is enough information */
		case DFP_PKT_PING: {
			if (dfp.base.dst_tank_x == tank_x && dfp.base.dst_tank_y == tank_y) { 
				/* turn this packet around... */
			    dfp.base.dst_tank_x = dfp.base.src_tank_x;
			    dfp.base.dst_tank_y = dfp.base.src_tank_y;
			    dfp.base.src_tank_x = tank_x;
			    dfp.base.src_tank_y = tank_y;
			    dfp.base.size = htons(DFP_SIZE_LOCATE);
			    dfp.base.type = DFP_PKT_PONG;
			    dfp.base.pad = 0;
			    dfp.base.version = htons(DFP_VERSION);
			
				sendto(dstSocket, (void *) &dfp, sizeof(dfp), 0, (sockaddr *) &dst, sizeof(dst) );
			}

			break;
		}
		case DFP_PKT_PONG: {
			if( (dfp.base.src_tank_x == tank_x -1) && dfp.base.src_tank_y == tank_y    ) {
				tank_left=true;
			}
			if( (dfp.base.src_tank_x == tank_x +1) && dfp.base.src_tank_y == tank_y    ) {
				tank_right=true;
			}
			if( (dfp.base.src_tank_x == tank_x   ) && dfp.base.src_tank_y == tank_y - 1) {
				tank_up=true;
			}
			if( (dfp.base.src_tank_x == tank_x   ) && dfp.base.src_tank_y == tank_y + 1) {
				tank_down=true;
			}
			BMessage msg(TANK_STATUS);
			msg.AddBool("tank_left", tank_left);
			msg.AddBool("tank_right", tank_right);
			msg.AddBool("tank_up", tank_up);
			msg.AddBool("tank_down", tank_down);
			Courier.SendMessage(&msg);
			break;
		}
		case DFP_PKT_ACK_FISH: {
			if( (dfp.base.dst_tank_x == tank_x) && (dfp.base.dst_tank_y == tank_y) ) {
				Fish *tempFish;
				tempFish = new Fish(&dfp);
				for(int i = 0; i < fishList.CountItems(); i++) {
					if (*tempFish == *((Fish *) fishList.ItemAt(i)) ){
					    
						( (Fish *) fishList.ItemAt(i))->Wrap(&dfp);
						fishList.RemoveItem(i);
					
					    dfp.base.dst_tank_x = dfp.base.src_tank_x; /* send it back */
					    dfp.base.dst_tank_y = dfp.base.src_tank_y;
					    dfp.base.type = DFP_PKT_SYNC_FISH; /* send it sync? */
					    dfp.base.src_tank_x = tank_x;
					    dfp.base.src_tank_y = tank_y;
		
					    dfp.base.size = htons(DFP_SIZE_TRANSFER);
					    dfp.base.pad = 0;
					    dfp.base.version = htons(DFP_VERSION);
					
						sendto(dstSocket, (void *) &dfp, sizeof(dfp), 0, (sockaddr *) &dst, sizeof(dst) );
						break; /* only SYNC one fish per ACK */
					}
				}
			}
			break;
		}
		case DFP_PKT_NACK_FISH: {
			break;
		}
		/* a SEND_FISH packet includes an offer of a fish -- reply with an ACK */
		case DFP_PKT_SEND_FISH: {
			if( (dfp.base.dst_tank_x == tank_x) && (dfp.base.dst_tank_y == tank_y) ) {
		
			    dfp.base.dst_tank_x = dfp.base.src_tank_x; /* send it back */
			    dfp.base.dst_tank_y = dfp.base.src_tank_y;
			    dfp.base.type = DFP_PKT_ACK_FISH; /* send it ack */
			    dfp.base.src_tank_x = tank_x;
			    dfp.base.src_tank_y = tank_y;
	
			    dfp.base.size = htons(DFP_SIZE_TRANSFER);
			    dfp.base.pad = 0;
			    dfp.base.version = htons(DFP_VERSION);
			
				sendto(dstSocket, (void *) &dfp, sizeof(dfp), 0, (sockaddr *) &dst, sizeof(dst) );
			}
			break;
		}
		case DFP_PKT_SYNC_FISH: {
			if( (dfp.base.dst_tank_x == tank_x) && (dfp.base.dst_tank_y == tank_y) ) {
				BMessage msg(SEND_FISH);
				msg.AddData("fish", DFP_TRANSFER_TYPE, &dfp, sizeof(dfp));
				Courier.SendMessage(&msg);
			}
			break;
		}
		default: {
			printf ("Someone is telling tall tales about their fish (unidentified packet)");
		}
		} /* end case */

    }
}

/*****************/

void 
FishPortal::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case PORTAL_INIT : {
			msg->FindInt8("tank_x", (int8 *) &tank_x);
			msg->FindInt8("tank_y", (int8 *) &tank_y);
			if (!pulseRunner) {
				pulseRunner = new BMessageRunner(BMessenger(this), new BMessage( PING_PULSE ), 800000);
			}
		break;
		}
		case NET_FISH : {
			/* look!  New Fish for the NET! */
			dfp_pkt_transfer	*dfp;
			ssize_t size;
			msg->FindData("fish", DFP_TRANSFER_TYPE, (const void **) &dfp, &size);
			/* check to see if it can escape */
			if(dfp->fish.x == 0) {
				if (tank_left) {
					dfp->base.dst_tank_x = tank_x -1;
					dfp->base.dst_tank_y = tank_y;
					SendFish(dfp); // send it.
					fishList.AddItem(new Fish(dfp)); // Add it to list
				} else {
					/* return it to the window, going the other direction */
					dfp->fish.dx = (-1 * dfp->fish.dx);
					BMessage rmsg(SEND_FISH);
					rmsg.AddData("fish", DFP_TRANSFER_TYPE, dfp, sizeof(*dfp));
					Courier.SendMessage(&rmsg);
				}
			break;
			}
			if(dfp->fish.y == 0) {
				if(tank_up) {
					dfp->base.dst_tank_x = tank_x;
					dfp->base.dst_tank_y = tank_y-1;
					SendFish(dfp); // send it.
					fishList.AddItem(new Fish(dfp)); // Add it to list
				} else {
					/* return it to the window, going the other direction */
					dfp->fish.dy = (-1 * dfp->fish.dy);
					BMessage rmsg(SEND_FISH);
					rmsg.AddData("fish", DFP_TRANSFER_TYPE, dfp, sizeof(*dfp));
					Courier.SendMessage(&rmsg);
				}
			break;
			}
			if(dfp->fish.x == 255) {
				if(tank_right) {
					dfp->base.dst_tank_x = tank_x +1;
					dfp->base.dst_tank_y = tank_y;
					SendFish(dfp); // send it.
					fishList.AddItem(new Fish(dfp)); // Add it to list
				} else {
					/* return it to the window, going the other direction */
					dfp->fish.dx = (-1 * dfp->fish.dx);
					BMessage rmsg(SEND_FISH);
					rmsg.AddData("fish", DFP_TRANSFER_TYPE, dfp, sizeof(*dfp));
					Courier.SendMessage(&rmsg);
				}
			}
			if(dfp->fish.y == 255) {
				if(tank_down) {
					dfp->base.dst_tank_x = tank_x;
					dfp->base.dst_tank_y = tank_y + 1;
					SendFish(dfp); // send it.
					fishList.AddItem(new Fish(dfp)); // Add it to list
				} else {
					/* return it to the window, going the other direction */
					dfp->fish.dy = (-1 * dfp->fish.dy);
					BMessage rmsg(SEND_FISH);
					rmsg.AddData("fish", DFP_TRANSFER_TYPE, dfp, sizeof(*dfp));
					Courier.SendMessage(&rmsg);
				}
			break;
			}
		break; // just in case
		}
		case PING_PULSE : {
		
			/* clear the current tank status variables */
			tank_left = false;
			tank_right = false;
			tank_up = false;
			tank_down = false;
			/* inform the target */
			BMessage msg(TANK_STATUS);
			msg.AddBool("tank_left", tank_left);
			msg.AddBool("tank_right", tank_right);
			msg.AddBool("tank_up", tank_up);
			msg.AddBool("tank_down", tank_down);
			Courier.SendMessage(&msg);
			/* send out some ping messages */
			PingTank(tank_x, tank_y - 1); /* bottom */
			PingTank(tank_x, tank_y + 1); /* top */
			PingTank(tank_x + 1, tank_y); /* right */
			PingTank(tank_x - 1, tank_y); /* left */
		break;
		}
		default: { //Always handle the defaults
				 BHandler::MessageReceived(msg);
		}

	} // end of switch							
}

