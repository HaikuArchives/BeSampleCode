/* voter.h */
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef VOTER_H
#define VOTER_H

#include <Locker.h>

class voter {
	public:				voter();
		virtual			~voter();

		//set the initial information and
		bool			Bribe(		const char *name,
									const char *server,
									const char * method,
									const char *request);
		
		//static thread entry function
		static int32	BribeMe(void *arg);
		
		//actual thread loop function
		int32			Bribed();

		//start the voter voting
		bool			Vote();

		//temporarily cease voting
		bool			Cease();

		//arrest the voter
		bool			Arrest();

		//specify the server, method and request
		bool			SetTarget(	const char *server,
									const char * method,
									const char *request);
		
		bool			SetBufferSize(uint32 bytes);

		const bool		Voting() const;
		const bool		Arrested() const;
		const uint32	Votes() const;
		const char *	Server() const;
		const char *	Method() const;
		const char *	Request() const;
		const bool		IsValid();
		const ssize_t	BufferSize() const;
		
	private:
		thread_id		fThread;
		bool			fArrested;
		bool			fVoting;
		uint32			fVotes;
		char *			fServer;
		char *			fMethod;
		char *			fRequest;
		BLocker 		fLock;
		uint32			fAddr;
		ssize_t			fBufferSize;
		
		ssize_t 		_send_vote(	int sock,
									char *buf,
									int buf_size);
									
		ssize_t 		_recv_result(int sock,
									char *buf,
									const ssize_t buf_size);	
		
};

#endif
