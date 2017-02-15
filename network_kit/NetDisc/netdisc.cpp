/*
A Simple Network Discovery Protocol

*/

// Copyright 2000, Be Incorporated.   All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#include <stdio.h>
#include <stdlib.h>
#include <socket.h>
#include <netdb.h>
#include <assert.h>
#include <SupportKit.h>


#define ND_ERR -1
#define ND_DBG -1
#define ND_CHG 2
#define ND_DBG_LVL 1
#define holler(a,b) if (((a) == -1) || ((a) > ND_DBG_LVL)) { printf b ; fflush(stdout); };

const char *ItoA(uint32 addr)
{
	struct in_addr ia;
	ia.s_addr = addr;
	return inet_ntoa(ia);
}


/*	A simple list.
	Death times are given as host order seconds till death, but
	are stored as (host order) absolute time of death.
*/
typedef struct MemberInfo {
	uint32	death;
	uint32	addr;
} MemberInfo;

class MemberList {
	public:
		MemberList() { mCount = 0;};
		~MemberList() { };
		uint32	Count() {return mCount;};
		bool	Add(uint32 addr, uint32 death);
		bool	Remove(uint32 addr, bool lock = true);
		bool	RemoveOldest(uint32 &addr);
		uint32	NextDeath();
		bool	Has(uint32 addr, uint32 death, bool lock = true);
		void	FillOut(MemberInfo **buf, uint32 *count, bool networkorder = true);
		
		void Dump();
	private:
		static const uint32 kMaxMembers = 64;

		BLocker		lock;
		MemberInfo	mList[kMaxMembers];
		uint32		mCount;
};

bool MemberList::Add(uint32 addr, uint32 death)
{
	lock.Lock();
	bool didit = false;
	
	assert(mCount != kMaxMembers);
	if (! Has(addr,death,false)) {
		mList[mCount].addr = addr;
		mList[mCount].death = death + real_time_clock();
		mCount++;
		didit = true;
	}
	lock.Unlock();
	return didit;
}

bool MemberList::Has(uint32 addr, uint32 death, bool dolock)
{
	if (dolock) lock.Lock();
	for (uint32 i = 0; i < mCount; i++) {
		if (mList[i].addr == addr) {
			mList[i].death = death + real_time_clock();
			if (dolock) lock.Unlock();
			return true;
		}
	}
	if (dolock) lock.Unlock();
	return false;
}

bool MemberList::Remove(uint32 addr, bool dolock)
{
	if (dolock) lock.Lock();

	for (uint32 i = 0; i < mCount; i++) {
		if (mList[i].addr == addr) {
			mList[i] = mList[mCount - 1];
			mCount--;
			if (dolock) lock.Unlock();
			return true;
		}
	}
	if (dolock) lock.Unlock();
	return false;
}

bool MemberList::RemoveOldest(uint32 &addr)
{
	lock.Lock();
	
	for (uint32 i = 0; i < mCount; i++) {
		if (mList[i].death <= real_time_clock()) {
			addr = mList[i].addr;
			mList[i] = mList[mCount - 1];
			mCount--;
			lock.Unlock();
			return true;
		}
	}
	
	lock.Unlock();
	return false;
}

uint32 MemberList::NextDeath()
{
	uint32 timetill;
	bool gotone = false;
	lock.Lock();

	MemberInfo oldest;
	oldest.death = ULONG_MAX;
	for (uint32 i = 0; i < mCount; i++) {
		if (mList[i].death < oldest.death) {
			oldest = mList[i];
			gotone = true;
		}
	}

	lock.Unlock();
	
	if (gotone) {
		timetill = oldest.death - real_time_clock();
		holler(ND_DBG,("Nextdeath %s in %u secs\n",ItoA(oldest.addr),timetill));
	} else {
		timetill = ULONG_MAX;
		holler(ND_DBG,("No next death\n"));
	}
	
	return timetill;
}

void MemberList::FillOut(MemberInfo **buf, uint32 *count, bool networkorder)
{
	lock.Lock();
	MemberInfo *memberlist = 0;
	
	if (mCount == 0) {
		holler(ND_DBG,("Wont fill out - membership is zero\n"));
		*count = 0;
		*buf = 0;
	}
	
	memberlist = (MemberInfo *) malloc(mCount * sizeof(MemberInfo));
	if (memberlist == 0) {
		holler(ND_ERR,("Couldnt get memory to fillout memberlist\n"));
		exit(-1);
	}
	
	for(uint32 i=0;i<mCount;i++) {
		memberlist[i].addr = mList[i].addr;
		if (networkorder) {
			memberlist[i].death = htonl(mList[i].death - real_time_clock());
		} else {
			memberlist[i].death = mList[i].death - real_time_clock();
		}
	}

	*count = mCount;
	*buf = memberlist;
	lock.Unlock();
}
		
void MemberList::Dump()
{
	lock.Lock();
	for (uint32 i=0;i<mCount;i++) {
		printf("%d : Address %s Death %u\n",i,ItoA(mList[i].addr),mList[i].death - real_time_clock());
	}
	if (mCount == 0) {
		printf("No Members but me!\n");
	}
	lock.Unlock();
}





class NetDiscovery {
public:
	NetDiscovery(uint16 port);
	~NetDiscovery();
	
	// Return the current list.
	void GetList(MemberInfo **mi, uint32 *count) {
		mList.FillOut(mi, count, false);
	};
	
	// Start the list request.
	void RefreshList(int strength = 1);
	
	
	// Ideas for the future:
	
	// This computer seems to be dead, remove it from the list
	// and optionally let everyone know they're gone.
	//void DeathHint();
	
	// Specify callback for list changes.
	// added, removed, timedout
	//void AddNotifier( (*notify)());
	
	
private:
	// Gritty network stuff.
	int		mSocket;
	uint16	mPort;
	uint32	mMyAddress;
	void	mSetupSocket();
	void 	mBroadcast(char *packet, uint32 len);
	
	typedef struct BasePacket {
		uint16	magic;
		uint16	version;
		uint32	id;
		uint32	flags;
		uint32	command;
	} BasePacket;

	typedef struct ListRequestPacket {
		struct BasePacket base;
		uint32 tries;
	} ListRequestPacket;

	typedef struct ListReplyPacket {
		struct BasePacket base;
		uint32 count;
		MemberInfo	members[1];
	} ListReplyPacket;

	typedef struct AnnouncePacket {
		struct BasePacket base;
		uint32 next_announcement;
	} AnnouncePacket;
	
	enum {
		CMD_ANNOUNCE = 0,
		CMD_GOODBYE = 1,
		CMD_LIST_REQ = 2,
		CMD_LIST_REPLY = 3
	};
	void	mInitPacket(BasePacket **packet, uint32 len, uint32 command);
	void	mDeletePacket(BasePacket *packet);
	uint32	mNextId();
	
	// Tuning knobs.
	static const bool kBroadcastDeathHint = true;
	
	// Consts.
	static const uint16 kMagic = 0x5038;
	static const uint16 kVersion = 1;
	
	// Max time between annoucements, in seconds.
	static const uint32 kMaxAnnounceInterval = 270;
	static const uint32	kAnnounceJitter = 30;
	static const uint32	kMaxListRequestTries = 4;
	static const uint32 kReceiveBufferSize = 512;

	uint32	mRandom(uint32 space);
	
	static int32	mStartRecv(void *);
	
	void	mBroadcast(BasePacket *packet, uint32 len);
	void	mSendAnnounce(uint32 next_announcement = kMaxAnnounceInterval);
	void	mSendListRequest(uint32 tries = 1);
	void	mSendListResponse();
	void	mSendGoodbye();
	
	status_t WaitFor(char *buf,int size,uint32 &fromaddr,uint32 time);

	void 	mCalculateNextEvent(uint32 &action,uint32 &waittime);
	void	mReceiveLoop();
	void	mHandleListRequest(uint32 fromaddress, ListRequestPacket *packet);
	void	mHandleListReply(uint32 fromaddress, ListReplyPacket *packet);
	bool	mReply(uint32 try_number);
	
	uint32	mNextAnnounce;
	
	MemberList mList;
	void	mAddMember(uint32 addr, uint32 death = kMaxAnnounceInterval+kAnnounceJitter);
	void	mRemoveMember(uint32 addr);
	void 	mRemoveOldest();
	
};

int32 NetDiscovery::mStartRecv(void *ptr)
{
	NetDiscovery *nd = (NetDiscovery *)ptr;
	nd->mReceiveLoop();
	return 0;
}

void NetDiscovery::RefreshList(int strength)
{
	// You'll have a better probability of replies if you try harder,
	// but you'll have to wait for it.
	if (strength > 1) {
		snooze((strength-1) * mRandom(1000000));
	}
	mSendListRequest(strength);
}


void NetDiscovery::mAddMember(uint32 addr, uint32 death)
{
	if (addr != mMyAddress) {
	
		if (death > (2*kMaxAnnounceInterval)) {
			holler(ND_DBG,("Normalizing %s death %d to %d!\n",ItoA(addr),death,kMaxAnnounceInterval));
			death = kMaxAnnounceInterval;
		} 
		
		if (mList.Add(addr,death + kAnnounceJitter)) {
			holler(ND_CHG,("Added addr %s\n",ItoA(addr)));
			mList.Dump();
		}
	}
}

void NetDiscovery::mRemoveMember(uint32 addr)
{
	if (mList.Remove(addr)) {
		holler(ND_CHG,("Removed addr %s\n",ItoA(addr)));
		mList.Dump();
	}
}

uint32 NetDiscovery::mNextId()
{
	static int32 global_id = 0;

	return (uint32) atomic_add(&global_id, 1);
}


NetDiscovery::NetDiscovery(uint16 port)
{
	holler(ND_DBG,("NetDiscovery coming up\n"));
	
	assert(kMaxAnnounceInterval > kAnnounceJitter);
	
	mPort = port;
	mSetupSocket();
	mSendAnnounce();
	
	resume_thread(spawn_thread(&mStartRecv,"listener",B_NORMAL_PRIORITY,this));
	
}

NetDiscovery::~NetDiscovery()
{
	holler(ND_DBG,("NetDiscovery going down\n"));
	mSendGoodbye();
	closesocket(mSocket);
}

void NetDiscovery::mSetupSocket()
{
	struct sockaddr sa;
	struct sockaddr_in *sain = (struct sockaddr_in *)&sa;
	int salen = sizeof(struct sockaddr);
	
	mSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (mSocket < 0) {
		holler(ND_ERR,("Couldnt create socket\n"));
		exit(-1);
	}
	sain->sin_addr.s_addr = INADDR_ANY;
	sain->sin_port = htons(mPort);
	if (bind(mSocket,&sa,salen) < 0) {
		holler(ND_ERR,("Couldnt bind\n"));
		exit(-1);
	}
	
	if (getsockname(mSocket,&sa,&salen) < 0) {
		holler(ND_ERR,("Couldnt get my address\n"));
		exit(-1);
	}
	
	mMyAddress = sain->sin_addr.s_addr;
	holler(ND_DBG,("My address is %s\n",ItoA(mMyAddress)));
	
}

void NetDiscovery::mBroadcast(BasePacket *packet, uint32 len)
{
	struct sockaddr sa;
	struct sockaddr_in *sain = (struct sockaddr_in *)&sa;
	int salen = sizeof(struct sockaddr);
	
	sain->sin_addr.s_addr = INADDR_BROADCAST;
	sain->sin_port = htons(mPort);

	if (sendto(mSocket,packet,len,0,&sa,salen) < 0) {
		holler(ND_ERR,("Couldnt send\n"));
		exit(-1);
	}
}

void NetDiscovery::mInitPacket(BasePacket **packet, uint32 len, uint32 command)
{
	assert(len >= sizeof(BasePacket));
	
	*packet = (BasePacket *) malloc(len);
	memset(*packet,0,len);
	
	if (*packet == 0) {
		holler(ND_ERR,("Out of memory\n"));
		exit(-1);
	}
	
	(*packet)->magic = htons(kMagic);
	(*packet)->version = htons(kVersion);
	(*packet)->id = htonl(mNextId());
	(*packet)->flags = 0;
	(*packet)->command = htonl(command);
	
}


void NetDiscovery::mDeletePacket(BasePacket *packet)
{
	assert(packet != 0);
	assert(packet->magic == htons(kMagic));
	assert(packet->version == htons(kVersion));
	
	free(packet);
}

void NetDiscovery::mSendAnnounce(uint32 next_announcement)
{
	AnnouncePacket *packet;
	
	mInitPacket((BasePacket **) & packet, sizeof(AnnouncePacket), CMD_ANNOUNCE);
	
	// Normalize the announcement time.
	if ((next_announcement <= 0) || (next_announcement > kMaxAnnounceInterval)) {
		next_announcement = kMaxAnnounceInterval;
	}
	next_announcement += mRandom(kAnnounceJitter);
	
	packet->next_announcement = htonl(next_announcement);
	
	mNextAnnounce = next_announcement + real_time_clock();
	
	// Send.
	holler(ND_DBG,("Announcing, next announcement in %d seconds\n",next_announcement));
	mBroadcast((BasePacket *) packet, sizeof(AnnouncePacket));

	mDeletePacket((BasePacket *) packet);		
}

void NetDiscovery::mSendGoodbye()
{
	BasePacket *packet;
	
	mInitPacket(& packet, sizeof(BasePacket), CMD_GOODBYE);

	mBroadcast((BasePacket *)packet, sizeof(BasePacket));
	
	mDeletePacket((BasePacket *)packet);
}

void NetDiscovery::mSendListRequest(uint32 tries)
{
	ListRequestPacket *packet;
	
	mInitPacket((BasePacket **) & packet, sizeof(ListRequestPacket), CMD_LIST_REQ);
	
	// Normalize.
	if ((tries < 0) || (tries > kMaxListRequestTries)) {
		tries = kMaxListRequestTries;
	}
	packet->tries = htonl(tries);
	
	// Send.
	holler(ND_DBG,("sending list request try %d\n",tries));
	mBroadcast((BasePacket *) packet, sizeof(ListRequestPacket));

	mDeletePacket((BasePacket *) packet);		
}

status_t NetDiscovery::WaitFor(char *buf,int size,uint32 &fromaddr,uint32 time)
{
	struct sockaddr from;
	int fromlen;
	struct sockaddr_in *from_in;

	struct timeval tv;
	fd_set set;
	int err;
	
	tv.tv_sec = time;
	tv.tv_usec = 0;


	FD_ZERO(&set);
	FD_SET(mSocket,&set);
	err = select(mSocket+1,	&set,0,0,&tv);
	if (err < 0) {
		holler(ND_DBG,("Select error %s\n",strerror(err)));
		return -1;
	}
	if (err == 0) {
		return B_TIMED_OUT;
	}

	memset(buf,0,size);
	int len = recvfrom(mSocket, buf, size, 0, &from, &fromlen);
	if (len < B_OK) {
		holler(ND_ERR,("Socket error %s\n",strerror(len)));
		exit(-1);
	}

	from_in = (struct sockaddr_in *)&from;
	fromaddr = from_in->sin_addr.s_addr;
		
	return len;
}

void NetDiscovery::mCalculateNextEvent(uint32 &action,uint32 &waittime)
{
	uint32 nextdeath;
	
	if (mList.Count() > 0) {
		nextdeath = mList.NextDeath();
		holler(ND_DBG,("nextdeath %d nextannounce %d\n",nextdeath,mNextAnnounce-real_time_clock()));
		if (nextdeath < (mNextAnnounce - real_time_clock())) {
			action = CMD_GOODBYE;
			waittime = nextdeath;
			holler(ND_DBG,("nextaction CMD_GOODBYE in %u secs\n",waittime));
			return;
		}
	}
	action = CMD_ANNOUNCE;
	waittime = mNextAnnounce - real_time_clock();
	holler(ND_DBG,("nextaction CMD_ANNOUNCE in %u secs\n",waittime));
	return;
}

void NetDiscovery::mRemoveOldest()
{
	uint32 who;
	
	if (mList.RemoveOldest(who)) {
		holler(ND_CHG,("%s dropped off net\n",ItoA(who)));
	}
}

void NetDiscovery::mReceiveLoop()
{
	int len;
	char buf[kReceiveBufferSize];
	BasePacket *packet = (BasePacket *)buf;
	uint32	fromaddress;
	
	holler(ND_DBG,("Beginning to listen on port %d\n",mPort));
	
	while (true) {
		uint32 nextaction,waittime;
		
		mCalculateNextEvent(nextaction,waittime);				

		len = WaitFor((char*)buf,kReceiveBufferSize,fromaddress,waittime);
		if (len < 0) {
			holler(ND_DBG,("wait timed out\n"));
			if (nextaction == CMD_ANNOUNCE) {
				mSendAnnounce();
				continue;
			} else {
				mRemoveOldest();
				continue;
			}
		}

		if ((fromaddress == mMyAddress) || (fromaddress == INADDR_BROADCAST)) {
			continue;
		}

		if (((unsigned int)len < sizeof(struct BasePacket)) ||
			(packet->magic != htons(kMagic)) ||
			(packet->version != htons(kVersion))) {
			// Bad packet,  drop it.
			holler(ND_DBG,("Got funky packet\n"));
			continue;
		}

		switch (ntohl(packet->command))
		{
			case CMD_ANNOUNCE:
			holler(ND_DBG,("Got CMD_ANNOUNCE from %s\n",ItoA(fromaddress)));
			mAddMember(fromaddress, ntohl(((AnnouncePacket *)packet)->next_announcement));
			break;
			
			case CMD_GOODBYE:
			holler(ND_DBG,("Got CMD_GOODBYE from %s\n",ItoA(fromaddress)));
			mRemoveMember(fromaddress);
			break;
			
			case CMD_LIST_REPLY:
			holler(ND_DBG,("Got CMD_LIST_REPLY from %s\n",ItoA(fromaddress)));
			mAddMember(fromaddress);
			mHandleListReply(fromaddress, (ListReplyPacket *) packet);
			break;
			
			case CMD_LIST_REQ:
			holler(ND_DBG,("Got CMD_LIST_REQ from %s\n",ItoA(fromaddress)));
			mAddMember(fromaddress);
			mHandleListRequest(fromaddress, (ListRequestPacket *) packet);
			break;

		}

	}

}

void NetDiscovery::mHandleListReply(uint32 fromaddress, ListReplyPacket *list)
{
	MemberInfo *ptr = (MemberInfo *) list->members;
	uint32 count = ntohl(list->count);

	holler(ND_DBG,("New List with %d entries\n",count));
	
	for (uint32 i = 0;i < count; i++) {
		assert(ptr->addr != 0);
		mAddMember(ptr->addr,ntohl(ptr->death));
		ptr++;
	}
	
}

uint32 NetDiscovery::mRandom(uint32 space)
{
	return (uint32) ((float) space) * ((float)rand())/((float)RAND_MAX);
}


bool NetDiscovery::mReply(uint32 try_number)
{
	int count;
	uint32 random_number;
	
	assert(try_number != 0);
	
	if (try_number > kMaxListRequestTries) {
		holler(ND_DBG,("Got a trynum of %d, setting to %d\n",try_number,kMaxListRequestTries));
		try_number = kMaxListRequestTries;
	}
	
	count = mList.Count();
	
	if (count == 1) {
		// The only member I know is the person who just asked for a list.
		// We always reply here.
		return true;
	}

		
	random_number = mRandom(count / (1 << try_number));
	holler(ND_ERR,("random: %d / %d  gave %d\n",count,(1 << try_number),random_number));
	if (random_number == 0) {
		return true;
	}
	
	// So : on the first try, try_number == 1.  Every member has a 1/count chance
	// of replying, so statistically only one reply should happen.  As try_number
	// goes up, the number of possible replies increases exponentially.
	
	
	return false;
}


void NetDiscovery::mHandleListRequest(uint32 fromaddress, ListRequestPacket *packet)
{

	if (mReply(ntohl(packet->tries))) {

		holler(ND_DBG,("Replying to list req\n"));
		ListReplyPacket *listpacket;
		MemberInfo *members = 0;
		uint32 count;
		
		mList.FillOut(&members,&count);
		
		uint32 packetlength = sizeof(ListReplyPacket) + (count - 1) * sizeof(MemberInfo);

		mInitPacket((BasePacket **)&listpacket, packetlength, CMD_LIST_REPLY);
		
		memcpy(listpacket->members,members,count * sizeof(MemberInfo));
		
		listpacket->count = htonl(count);
	
		// Send.
		mBroadcast((BasePacket *)listpacket, packetlength);

		free(members);
	} else {
		holler(ND_DBG,("NOT Replying to list req\n"));
	}
}


int main(int argc, char **argv)
{
	NetDiscovery *nd;
	printf("Starting up...\n");

	nd = new NetDiscovery(6787);
	snooze(1000000);
	nd->RefreshList();
	
	if (argc > 1) {
		snooze(1000000 * atoi(argv[1]));
	} else {
		snooze(600000000);	
	}
	delete nd;
	
	printf("Leaving\n");
	return 0;
}